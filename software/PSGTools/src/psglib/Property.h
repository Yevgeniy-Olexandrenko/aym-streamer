#pragma once

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

#define  RW_PROP_DEF(TYPE, NAME)                           \
public:  const TYPE& NAME() const { return m_##NAME; };    \
public:  void NAME(const TYPE& NAME) { m_##NAME = NAME; }; \
private: TYPE m_##NAME{}

#define  RO_PROP_DEF(TYPE, NAME)                           \
public:  const TYPE& NAME() const { return m_##NAME; };    \
private: TYPE m_##NAME{}

#define  WO_PROP_DEF(TYPE, NAME)                           \
public:  void NAME(const TYPE& NAME) { m_##NAME = NAME; }; \
private: TYPE m_##NAME{}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

#define  RW_PROP_IMP(TYPE, NAME)      \
public:  TYPE NAME() const;           \
public:  void NAME(const TYPE& NAME); \
private: TYPE m_##NAME{}

#define  RO_PROP_IMP(TYPE, NAME)      \
public:  TYPE NAME() const;           \
private: TYPE m_##NAME{}

#define  WO_PROP_IMP(TYPE, NAME)      \
public:  void NAME(const TYPE& NAME); \
private: TYPE m_##NAME{}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

#define  RW_PROP_DEC(TYPE, NAME)      \
public:  TYPE NAME() const;           \
public:  void NAME(const TYPE& NAME);

#define  RO_PROP_DEC(TYPE, NAME)      \
public:  TYPE NAME() const;

#define  WO_PROP_DEC(TYPE, NAME)      \
public:  void NAME(const TYPE& NAME);
