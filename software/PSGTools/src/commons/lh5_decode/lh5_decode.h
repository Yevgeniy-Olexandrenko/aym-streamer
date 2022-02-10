#pragma once

// LHA5 decoder
extern bool lh5_decode(
	unsigned char* inp,
	unsigned char* outp,
	unsigned long original_size,
	unsigned long packed_size
);