////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: Placeholder, WorkInProgress
//
//#ifndef COGS_HEADER_GUI_BUTTONBOX
//#define COGS_HEADER_GUI_BUTTONBOX
//
//
//#include "cogs/collections/container_dlist.hpp"
//#include "cogs/gui/pane.hpp"
//#include "cogs/gui/pane_bridge.hpp"
//#include "cogs/gui/label.hpp"
//
//
//namespace cogs {
//namespace gui {
//
//
//class button_box_interface : public bridgeable_pane
//{
//public:
//	// Derived class may derive from btn for its own use.
//	class button : public object
//	{
//	public:
//		virtual void set_action(const function<void()>&) = 0;
//		virtual void set_text(const composite_string&) = 0;
//		virtual void set_enabled(bool) = 0;
//
//		explicit button(rc_obj_base& desc)
//		: object(desc)
//		{
//		}
//	};
//
//	virtual void nest(const rcref<pane>& contentPanel) = 0;
//
//	virtual rcref<button> append_button(const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false) = 0;
//	virtual rcref<button> prepend_button(const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false) = 0;
//	virtual rcref<button> insert_button_before(const rcref<button>& before, const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false) = 0;
//	virtual rcref<button> insert_button_after(const rcref<button>& after, const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false) = 0;
//	virtual void set_default_button(const rcptr<button>& btn) = 0;
//		
//	virtual void remove(const rcref<button>& btn) = 0;
//	virtual void clear_buttons() = 0;
//};
//
//
//// A button_box is a simple UI dialog that presents a primary frame, and in a secondary
//// frame displays a list of buttons.  Those buttons may be enabled/disable, and one may
//// be a default button if applicable.  
//// i.e. OK message box, if 1 button.  Yes/No, Action/Cancel dialog, if 2 buttons.  etc.
//class button_box : public pane_bridge<button_box_interface>
//{
//public:
//	class button;
//
//private:
//	typedef pane_bridge<button_box_interface> base_t;
//
//	rcref<pane> m_childPanel;
//
//	container_dlist<rcref<button> > m_buttonList;
//	rcptr<button> m_defaultButton;
//	size_t m_numButtons;
//
//public:
//	button_box()
//		: m_childPanel(rcnew(pane)),
//		m_numButtons(0)
//	{ }
//	
//	~button_box()
//	{ }
//
//	void nest(const rcref<pane>& child, const rcptr<frame>& f = 0)
//	{
//		m_childPanel->nest(child, f);
//	}
//
//	class button : public object
//	{
//	private:
//		function<void(const rcref<button>&)> m_action;
//		composite_string m_text;
//		bool m_isEnabled;
//		gfx::font m_font;
//		
//		weak_rcptr<button_box> m_buttonBox;
//		container_dlist<rcref<button> >::remove_token m_removeToken;
//		rcptr<button_box_interface::button> m_bridgedButton;
//
//	protected:
//		friend class button_box;
//
//		button(rc_obj_base& desc, const rcref<button_box>& bb, const function<void(const rcref<button>&)>& m_action, const composite_string& text, const gfx::font& fnt, bool isEnabled)
//			: object(desc),
//			m_buttonBox(bb),
//			m_text(text),
//			m_font(fnt),
//			m_isEnabled(isEnabled)
//		{ }
//
//	public:
//		void remove()
//		{
//			rcptr<button_box> buttonBox = m_buttonBox;
//			if (!!buttonBox)
//				buttonBox->remove(this_rcref);
//		}
//
//		void set_text(const composite_string& s)
//		{
//			m_text = s;
//			rcptr<button_box_interface::button> bridgedButton = m_bridgedButton;
//			if (!!bridgedButton)
//				bridgedButton->set_text(s);
//		}
//
//		composite_string get_text() const { return m_text; }
//
//		void set_enabled(bool b)
//		{
//			if (m_isEnabled != b)
//			{
//				m_isEnabled = b;
//				rcptr<button_box_interface::button> bridgedButton = m_bridgedButton;
//				if (!!bridgedButton)
//					bridgedButton->set_enabled(b);
//			}
//		}
//
//		bool get_enabled() { return m_isEnabled; }
//
//		void set_default(bool b = true)
//		{
//			rcptr<button_box> buttonBox = m_buttonBox;
//			if (!!buttonBox)
//			{
//				if (b)
//					buttonBox->set_default_button(this_rcref);
//				else // if (!b) // if clearing only if equals
//				{
//					const rcptr<button>& currentDefault = buttonBox->get_default_button();
//					if (currentDefault == this_rcref)
//						buttonBox->clear_default_button();
//				}
//			}
//		}
//
//		bool is_default()
//		{
//			bool b = false;
//			rcptr<button_box> buttonBox = m_buttonBox;
//			if (!!buttonBox)
//				b = (buttonBox->get_default_button() == this_rcref);
//			return b;
//		}
//
//		const function<void(const rcref<button>&)>& get_action() const { return m_action; }
//		void set_action(const function<void(const rcref<button>&)>& d)
//		{
//			m_action = d;
//			rcptr<button_box_interface::button> bridgedButton = m_bridgedButton;
//			if (!!bridgedButton)
//			{
//				bridgedButton->set_action([r{ this_weak_rcptr }]()
//				{
//					rcptr<button> r2 = r;
//					if (!!r2)
//						r2->m_action(r2.dereference());
//				});
//			}
//		}
//	};
//
//	void remove(const rcref<button>& b)
//	{
//		if (m_defaultButton == b)
//			m_defaultButton.release();
//		--m_numButtons;
//		m_buttonList.remove(b->m_removeToken);
//		rcptr<button_box_interface> buttonBox = get_bridged();
//		if (!!buttonBox)
//			buttonBox->remove(b->m_bridgedButton.dereference());
//	}
//
//	void set_default_button(const rcptr<button>& b)
//	{
//		if (m_defaultButton != b)
//		{
//			m_defaultButton = b;
//			rcptr<button_box_interface> buttonBox = get_bridged();
//			if (!!buttonBox)
//				buttonBox->set_default_button(b->m_bridgedButton.dereference());
//		}
//	}
//
//	void clear_default_button() { set_default_button(rcptr<button>()); }
//
//	const rcptr<button>& get_default_button() const { return m_defaultButton; }
//
//
//	rcref<button> append_button(const function<void(const rcref<button>&)>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		m_numButtons++;
//		rcref<button> b = rcnew(button, this_rcref, action, text, fnt, isEnabled);
//		if (isDefault)
//			m_defaultButton = b;
//		m_buttonList.append(b);
//		rcptr<button_box_interface> buttonBox = get_bridged();
//		if (!!buttonBox)
//		{
//			rcref<button_box_interface::button> bridgedButton = buttonBox->append_button([r{ b.weak() }]()
//			{
//				rcptr<button> r2 = r;
//				if (!!r2)
//					r2->m_action(r2.dereference());
//			}, text, fnt, isEnabled, isDefault);
//			b->m_bridgedButton = bridgedButton;
//		}
//		return b;
//	}
//
//	rcref<button> prepend_button(const function<void(const rcref<button>&)>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		m_numButtons++;
//		rcref<button> b = rcnew(button, this_rcref, action, text, fnt, isEnabled);
//		if (isDefault)
//			m_defaultButton = b;
//		m_buttonList.prepend(b);
//		rcptr<button_box_interface> buttonBox = get_bridged();
//		if (!!buttonBox)
//		{
//			rcref<button_box_interface::button> bridgedButton = buttonBox->prepend_button([r{ b.weak() }]()
//			{
//				rcptr<button> r2 = r;
//				if (!!r2)
//					r2->m_action(r2.dereference());
//			}, text, fnt, isEnabled, isDefault);
//			b->m_bridgedButton = bridgedButton;
//			if (isDefault)
//				buttonBox->set_default_button(bridgedButton);
//		}
//		return b;
//	}
//
//	rcref<button> insert_button_before(const function<void(const rcref<button>&)>& action, const rcref<button>& before, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		m_numButtons++;
//		rcref<button> b = rcnew(button, this_rcref, action, text, fnt, isEnabled);
//		if (isDefault)
//			m_defaultButton = b;
//		m_buttonList.insert_before(b, before->m_removeToken);
//		rcptr<button_box_interface> buttonBox = get_bridged();
//		if (!!buttonBox)
//		{
//			rcref<button_box_interface::button> bridgedButton = buttonBox->insert_button_before(before->m_bridgedButton.dereference(), [r{ b.weak() }]()
//			{
//				rcptr<button> r2 = r;
//				if (!!r2)
//					r2->m_action(r2.dereference());
//			}, text, fnt, isEnabled, isDefault);
//			b->m_bridgedButton = bridgedButton;
//			if (isDefault)
//				buttonBox->set_default_button(bridgedButton);
//		}
//		return b;
//	}
//
//	rcref<button> insert_button_after(const function<void(const rcref<button>&)>& action, const rcref<button>& after, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		m_numButtons++;
//		rcref<button> b = rcnew(button, this_rcref, action, text, fnt, isEnabled);
//		if (isDefault)
//			m_defaultButton = b;
//		m_buttonList.insert_after(b, after->m_removeToken);
//		rcptr<button_box_interface> buttonBox = get_bridged();
//		if (!!buttonBox)
//		{
//			rcref<button_box_interface::button> bridgedButton = buttonBox->insert_button_after(after->m_bridgedButton.dereference(), [r{ b.weak() }]()
//			{
//				rcptr<button> r2 = r;
//				if (!!r2)
//					r2->m_action(r2.dereference());
//			}, text, fnt, isEnabled, isDefault);
//			b->m_bridgedButton = bridgedButton;
//			if (isDefault)
//				buttonBox->set_default_button(bridgedButton);
//		}
//		return b;
//	}
//
//	void clear_buttons()
//	{
//		m_numButtons = 0;
//		m_defaultButton.release();
//		m_buttonList.clear();
//		rcptr<button_box_interface> buttonBox = get_bridged();
//		if (!!buttonBox)
//			buttonBox->clear_buttons();
//	}
//
//	size_t get_num_buttons() { return m_numButtons; }
//
//	const container_dlist<rcref<button> >& get_button_list() const { return m_buttonList; }
//
//	virtual void installing()
//	{
//		rcref<button_box_interface> buttonBox = get_subsystem()->create_button_box().template static_cast_to<button_box_interface>();
//		
//		base_t::install_bridged(buttonBox);
//
//		container_dlist<rcref<button> >::iterator itor = m_buttonList.get_first();
//		while (!!itor)
//		{
//			const rcref<button>& b = *itor;
//			b->m_bridgedButton = buttonBox->append_button([r{ b.weak() }]()
//			{
//				rcptr<button> r2 = r;
//				if (!!r2)
//					r2->m_action(r2.dereference());
//			}, b->m_text, b->m_font, b->m_isEnabled, b->is_default());
//			++itor;
//		}
//		buttonBox->nest(m_childPanel);
//	}
//};
//
//
//class message_box : public button_box
//{
//private:
//	rcref<button_box::button> m_okButton;
//	function<void()> m_okDelegate;
//	rcref<label> m_messageText;
//
//	virtual void detach(const rcref<button>& b)
//	{
//		button_box::detach();
//	}
//
//public:
//	message_box(const composite_string& msg, const function<void()>& d, const composite_string& text = composite_string::literal(L"OK"), const gfx::font& fnt = gfx::font(), bool isEnabled = true)
//		: m_messageText(rcnew(label, msg)),
//			m_okButton(append_button(d, text, fnt, isEnabled, true))
//	{
//		nest(m_messageText);
//	}
//	
//	explicit message_box(const composite_string& msg, const composite_string& text = composite_string::literal(L"OK"), const gfx::font& fnt = gfx::font(), bool isEnabled = true)
//		: m_messageText(rcnew(label, msg)),
//		m_okButton(append_button([r{ this_weak_rcptr }](const rcref<button>& b)
//		{
//			rcptr<message_box> r2 = r;
//			if (!!r2)
//				r2->detach(b);
//		}, text, fnt, isEnabled, true))
//	{
//		nest(m_messageText);
//	}
//
////	void set_text(const composite_string& s) { m_messageText->set_text(s); }
////	composite_string get_text() const { return m_messageText->get_text(); }
//
//	const rcref<button> get_button() const { return m_okButton; }
//	const rcref<label> get_label() const { return m_messageText; }
//
//	const function<void()>& get_action() const { return m_okDelegate; }
//	void set_action(const function<void()>& d) { m_okDelegate = d; m_okButton->set_action(d); }
//
////	void set_button_text(const composite_string& s) { m_okButton->set_text(s); }
////	composite_string get_button_text() const { return m_okButton->get_text(); }
//
////	void set_button_enabled(bool b) { m_okButton->set_enabled(b); }
////	bool get_button_enabled() { return m_okButton->get_enabled(); }
//};
//
//
//
//class two_button_box
//{
//public:
//};
//
//class yes_no_prompt : public two_button_box
//{
//public:
//};
//
//

//class default_button_box_button : public button_box_interface::button
//{
//public:
//	wrap_list<script_flow::right_to_left_top_to_bottom>::nest_token m_nestToken;
//	rcref<gui::button> m_uiButton;
//
//	default_button_box_button(const rcref<gui::button>& uiButton)
//		: m_uiButton(uiButton)
//	{ }
//
//	virtual void set_action(const function<void()>& d) { m_uiButton->set_action(d); }
//	virtual void set_text(const composite_string& s) { m_uiButton->set_text(s); }
//	virtual void set_enabled(bool b) { m_uiButton->set_enabled(b); }
//};
//
//
//class default_button_box : public button_box_interface
//{
//private:
//	rcptr<default_button_box_button> m_defaultButton;
//	rcref<grid<> > m_grid;
//	rcref<wrap_list<script_flow::right_to_left_top_to_bottom> > m_wrapList;
//
//public:
//	default_button_box()
//		: m_grid(rcnew(grid<>)),
//			m_wrapList(rcnew(wrap_list<script_flow::right_to_left_top_to_bottom>, 0.5, 0.5))
//	{
//		m_grid->nest(m_wrapList, 0, 1);
//	}
//	
//	virtual void nest(const rcref<pane>& contentPane)
//	{
//		m_grid->nest(contentPane, 0, 0);
//	}
//
//	virtual rcref<button_box_interface::button> append_button(const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button> uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		m_wrapList->nest_last(uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual rcref<button_box_interface::button> prepend_button(const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button> uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		m_wrapList->nest_first(uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual rcref<button_box_interface::button> insert_button_before(const rcref<button_box_interface::button>& before, const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button> uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		rcref<default_button_box_button> beforeBtn = before.template static_cast_to<default_button_box_button>();
//		m_wrapList->nest_before(uiButton, beforeBtn->m_uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual rcref<button_box_interface::button> insert_button_after(const rcref<button_box_interface::button>& after, const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button> uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		rcref<default_button_box_button> afterBtn = after.template static_cast_to<default_button_box_button>();
//		m_wrapList->nest_before(uiButton, afterBtn->m_uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual void set_default_button(const rcptr<button_box_interface::button>& btn)
//	{
//		rcptr<default_button_box_button> b = btn.template static_cast_to<default_button_box_button>();
//		if (m_defaultButton != b)
//		{
//			if (!!m_defaultButton)
//				m_defaultButton->m_uiButton->set_default(false);
//			m_defaultButton = b;
//			if (!!b)
//				b->m_uiButton->set_default(true);
//		}
//	}
//	
//	virtual void remove(const rcref<button_box_interface::button>& btn)
//	{
//		rcref<default_button_box_button> b = btn.template static_cast_to<default_button_box_button>();
//		if (m_defaultButton == b)
//			m_defaultButton.release();
//		b->m_uiButton->detach();
//	}
//
//	virtual void clear_buttons()
//	{
//		m_defaultButton.release();
//		m_wrapList->clear();
//	}
//
//	virtual void installing()
//	{
//		rcptr<gui::button_box> bb = get_bridge().template static_cast_to<gui::button_box>();
//		bb->pane::nest(m_wrapList);
//		//bb->panel::nest(m_grid);
//		button_box_interface::installing();
//	}
//
//	virtual void uninstalling()
//	{
//		m_wrapList->detach();
//		//m_grid->detach();
//		button_box_interface::uninstalling();
//	}
//};
//
//
//rcref<button_box_interface> subsystem::create_button_box() volatile
//{
//	return rcnew(default_button_box);
//}
//
//
//}
//}
//
//
//#endif
