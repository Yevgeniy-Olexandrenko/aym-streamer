#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "src/psg/mode/psg_turbosound.h"
#include "src/uart/uart_protocol.h"

// the streamer type: PSG::Basic or PSG::Advanced
using PSGMode = PSG::Advanced;

PSG::TurboSound<PSGMode> m_psg;
UART::Protocol<PSGMode> m_protocol;

// The config saved by the 'save' command, applied at the start. It is
// for hosts that do not send the config. A host config overrides it.
struct config_t
{
	uint8_t     magic;
	PSG::Stereo stereo;
	PSG::Clock  clock;
	uint8_t     check;
};

static constexpr uint8_t CONFIG_MAGIC = 0xA5;
static config_t EEMEM s_config;

static uint8_t config_check(const config_t& config)
{
	// a sum of all bytes before the check, an erased EEPROM is 0xFF
	const uint8_t* data = reinterpret_cast<const uint8_t*>(&config);
	uint8_t sum = 0;
	for (uint8_t i = 0; i < offsetof(config_t, check); ++i) sum += data[i];
	return ~sum;
}

static void load_config()
{
	config_t config;
	eeprom_read_block(&config, &s_config, sizeof(config));
	if (config.magic == CONFIG_MAGIC && config.check == config_check(config) &&
		config.stereo <= PSG::Stereo::CBA)
	{
		m_protocol.Configure(m_psg, config.stereo, config.clock);
	}
}

static void save_config()
{
	// the config the streamer uses: for Basic the real clock and ABC
	config_t config;
	config.magic = CONFIG_MAGIC;
	m_protocol.GetConfig(m_psg, config.stereo, config.clock);
	config.check = config_check(config);

	// the update writes only the changed bytes, 3.3 ms each
	eeprom_update_block(&config, &s_config, sizeof(config));
	UART::WriteConfig(config.stereo, config.clock);
	UART::WriteLineEnd();
}

static void send_chips()
{
	// 'E6D5F1AE - AY8910' for each chip
	PSG::ChipId chips[2];
	m_protocol.GetChips(m_psg, chips[0], chips[1]);
	for (PSG::ChipId chip : chips)
	{
		UART::WriteHex(uint32_t(chip));
		UART::WriteText(PSTR(" - "));
		UART::WriteText(UART::ChipName(chip));
		UART::WriteLineEnd();
	}
}

// The text commands for a terminal, they are not a part of the protocol:
// 'info' - the reply of the config command without a change,
// 'chip' - the ChipId hash and the type of each chip,
// 'save' - save the config in use for the next starts, reply 'ABC1773400'.
static void handle_text(const char* text)
{
	if (!strcmp_P(text, PSTR("info")))
	{
		m_protocol.SendInfo(m_psg);
	}
	else if (!strcmp_P(text, PSTR("chip")))
	{
		send_chips();
	}
	else if (!strcmp_P(text, PSTR("save")))
	{
		save_config();
	}
}

void setup()
{
	m_psg.PowerOn();
	load_config();
	m_protocol.Start(handle_text);
}

void loop()
{
	m_protocol.Process(m_psg);
}
