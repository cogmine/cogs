//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_UI_KEYBOARD
#define COGS_HEADER_UI_KEYBOARD

#include <bitset>


namespace cogs {

/// @brief Namespace for UI
namespace ui {

/// @brief Keyboard keys that do not have standard unicode identifiers
namespace special_keys {

static constexpr wchar_t TabKey = 0x0009;
static constexpr wchar_t EscapeKey = 0x001B;
static constexpr wchar_t ReturnKey = 0x000D;
static constexpr wchar_t UpArrowKey = 0xF700;
static constexpr wchar_t DownArrowKey = 0xF701;
static constexpr wchar_t LeftArrowKey = 0xF702;
static constexpr wchar_t RightArrowKey = 0xF703;
static constexpr wchar_t F1Key = 0xF704;
static constexpr wchar_t F2Key = 0xF705;
static constexpr wchar_t F3Key = 0xF706;
static constexpr wchar_t F4Key = 0xF707;
static constexpr wchar_t F5Key = 0xF708;
static constexpr wchar_t F6Key = 0xF709;
static constexpr wchar_t F7Key = 0xF70A;
static constexpr wchar_t F8Key = 0xF70B;
static constexpr wchar_t F9Key = 0xF70C;
static constexpr wchar_t F10Key = 0xF70D;
static constexpr wchar_t F11Key = 0xF70E;
static constexpr wchar_t F12Key = 0xF70F;
static constexpr wchar_t F13Key = 0xF710;
static constexpr wchar_t F14Key = 0xF711;
static constexpr wchar_t F15Key = 0xF712;
static constexpr wchar_t F16Key = 0xF713;
static constexpr wchar_t F17Key = 0xF714;
static constexpr wchar_t F18Key = 0xF715;
static constexpr wchar_t F19Key = 0xF716;
static constexpr wchar_t F20Key = 0xF717;
static constexpr wchar_t F21Key = 0xF718;
static constexpr wchar_t F22Key = 0xF719;
static constexpr wchar_t F23Key = 0xF71A;
static constexpr wchar_t F24Key = 0xF71B;
static constexpr wchar_t F25Key = 0xF71C;
static constexpr wchar_t F26Key = 0xF71D;
static constexpr wchar_t F27Key = 0xF71E;
static constexpr wchar_t F28Key = 0xF71F;
static constexpr wchar_t F29Key = 0xF720;
static constexpr wchar_t F30Key = 0xF721;
static constexpr wchar_t F31Key = 0xF722;
static constexpr wchar_t F32Key = 0xF723;
static constexpr wchar_t F33Key = 0xF724;
static constexpr wchar_t F34Key = 0xF725;
static constexpr wchar_t F35Key = 0xF726;
static constexpr wchar_t InsertKey = 0xF727;
static constexpr wchar_t DeleteKey = 0xF728;
static constexpr wchar_t HomeKey = 0xF729;
static constexpr wchar_t BeginKey = 0xF72A;
static constexpr wchar_t EndKey = 0xF72B;
static constexpr wchar_t PageUpKey = 0xF72C;
static constexpr wchar_t PageDownKey = 0xF72D;
static constexpr wchar_t PrintScreenKey = 0xF72E;
static constexpr wchar_t ScrollLockKey = 0xF72F;
static constexpr wchar_t PauseKey = 0xF730;
static constexpr wchar_t SysReqKey = 0xF731;
static constexpr wchar_t BreakKey = 0xF732;
static constexpr wchar_t ResetKey = 0xF733;
static constexpr wchar_t StopKey = 0xF734;
static constexpr wchar_t MenuKey = 0xF735;
static constexpr wchar_t UserKey = 0xF736;
static constexpr wchar_t SystemKey = 0xF737;
static constexpr wchar_t PrintKey = 0xF738;
static constexpr wchar_t ClearLineKey = 0xF739;
static constexpr wchar_t ClearDisplayKey = 0xF73A;
static constexpr wchar_t InsertLineKey = 0xF73B;
static constexpr wchar_t DeleteLineKey = 0xF73C;
static constexpr wchar_t InsertCharKey = 0xF73D;
static constexpr wchar_t DeleteCharKey = 0xF73E;
static constexpr wchar_t PrevKey = 0xF73F;
static constexpr wchar_t NextKey = 0xF740;
static constexpr wchar_t SelectKey = 0xF741;
static constexpr wchar_t ExecuteKey = 0xF742;
static constexpr wchar_t UndoKey = 0xF743;
static constexpr wchar_t RedoKey = 0xF744;
static constexpr wchar_t FindKey = 0xF745;
static constexpr wchar_t HelpKey = 0xF746;
static constexpr wchar_t ModeSwitchKey = 0xF747;

};


enum class physical_modifier_key {
	left_shift_key = 2,
	right_shift_key = 3,
	left_control_key = 8,
	right_control_key = 9,
	left_alt_key = 10,
	right_alt_key = 11,
	left_command_key = 12,
	right_command_key = 13
};

enum class modifier_key {
	shift_key = 1,
	control_key = 4,
	alt_key = 5,
	command_key = 6
};

enum class toggleable_modifier_key {
	caps_lock_key = 0,
	num_lock_key = 7
};

class modifier_keys_state
{
private:
	std::bitset<14> m_bitset;

public:
	bool operator==(const modifier_keys_state& cmp) const { return m_bitset == cmp.m_bitset; }
	bool operator!=(const modifier_keys_state& cmp) const { return m_bitset != cmp.m_bitset; }


	bool get_key(modifier_key key) const { return m_bitset[(size_t)key]; }

	bool get_key(toggleable_modifier_key key) const { return m_bitset[(size_t)key]; }

	bool get_key(physical_modifier_key key) const { return m_bitset[(size_t)key]; }


	void set_key(toggleable_modifier_key key, bool b) { m_bitset.set((size_t)key, b); }

	void set_key(physical_modifier_key key, bool b)
	{
		m_bitset.set((size_t)key, b);
		size_t modifierKey = (size_t)key >> 1; // modifier_key is (physical_modifier_key / 2)
		if (b)
			m_bitset.set(modifierKey, true);
		else
		{
			size_t otherSideKey = (size_t)key ^ 0x01;
			if (!m_bitset[otherSideKey])
				m_bitset.set(modifierKey, false);
		}
	}
};


}
}


#endif
