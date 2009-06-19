#ifndef __MODEST_MSG_VIEW_WINDOW_UI_DIMMING_PRIV_H__
#define __MODEST_MSG_VIEW_WINDOW_UI_DIMMING_PRIV_H__

#include "modest-dimming-rules-group.h"
#include "modest-ui-dimming-rules.h"

G_BEGIN_DECLS


/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_view_menu_dimming_entries [] = {

	/* Message Menu */
	{ "/MenuBar/MessageMenu/MessageNewMenu", G_CALLBACK(modest_ui_dimming_rules_on_new_msg) },
	{ "/MenuBar/MessageMenu/MessageReplyMenu", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/MenuBar/MessageMenu/MessageReplyAllMenu", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/MenuBar/MessageMenu/MessageForwardMenu",  G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/MenuBar/MessageMenu/MessageDeleteMenu",  G_CALLBACK(modest_ui_dimming_rules_on_delete_msg) },
	{ "/MenuBar/MessageMenu/MessageDetailsMenu", G_CALLBACK(modest_ui_dimming_rules_on_details) },

	/* Edit Menu */
	{ "/MenuBar/EditMenu", NULL },
	{ "/MenuBar/EditMenu/EditPasteMenu", G_CALLBACK(modest_ui_dimming_rules_always_dimmed) },
	{ "/MenuBar/EditMenu/EditSelectAllMenu", NULL },
	{ "/MenuBar/EditMenu/EditMoveToMenu", G_CALLBACK(modest_ui_dimming_rules_on_move_to) },

	/* View Menu */
	{ "/MenuBar/ViewMenu", NULL },
	{ "/MenuBar/ViewMenu/ZoomMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewToggleFullscreenMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewPreviousMessageMenu", G_CALLBACK(modest_ui_dimming_rules_on_view_previous) },
	{ "/MenuBar/ViewMenu/ViewNextMessageMenu",  G_CALLBACK(modest_ui_dimming_rules_on_view_next)},
	
	/* Attachments Menu */
	{ "/MenuBar/AttachmentsMenu", NULL },
	{ "/MenuBar/AttachmentsMenu/ViewAttachmentMenu", G_CALLBACK(modest_ui_dimming_rules_on_view_attachments) },
	{ "/MenuBar/AttachmentsMenu/SaveAttachmentMenu", G_CALLBACK(modest_ui_dimming_rules_on_save_attachments) },
	{ "/MenuBar/AttachmentsMenu/RemoveAttachmentMenu", G_CALLBACK(modest_ui_dimming_rules_on_remove_attachments) },

	/* Tools Menu */
	{ "/MenuBar/ToolsMenu", NULL },

	/* Close Menu */
	{ "/MenuBar/CloseMenu", NULL },
	{ "/MenuBar/ToolsMenu/CloseWindowMenu", NULL },
	{ "/MenuBar/ToolsMenu/CloseAllWindowsMenu", NULL },

	/* Contextual Menus (Toolbar) */
	{ "/ToolbarReplyCSM/MessageForwardMenu", NULL },
	{ "/ToolbarReplyCSM/MessageReplyAllMenu", NULL },
	{ "/ToolbarReplyCSM/MessageReplyMenu", NULL },
	
};

/* Clipboard status dimming rule entries */
static const ModestDimmingEntry modest_msg_view_clipboard_dimming_entries [] = {
	{ "/MenuBar/EditMenu/EditCutMenu", G_CALLBACK(modest_ui_dimming_rules_always_dimmed) },
	{ "/MenuBar/EditMenu/EditCopyMenu", G_CALLBACK(modest_ui_dimming_rules_on_copy) },
	{ "/MenuBar/ToolsMenu/ToolsAddToContactsMenu", G_CALLBACK (modest_ui_dimming_rules_on_add_to_contacts) },
};

/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_view_toolbar_dimming_entries [] = {

	/* Toolbar */
	{ "/ToolBar/ToolbarMessageReply", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/ToolBar/ToolbarMessageMoveTo", G_CALLBACK(modest_ui_dimming_rules_on_move_to) },
	{ "/ToolBar/ToolbarDeleteMessage",  G_CALLBACK(modest_ui_dimming_rules_on_delete_msg) },
	{ "/ToolBar/FindInMessage", G_CALLBACK(modest_ui_dimming_rules_on_find_in_msg) },
	{ "/ToolBar/ToolbarMessageBack", G_CALLBACK(modest_ui_dimming_rules_on_view_previous) },
	{ "/ToolBar/ToolbarMessageNext", G_CALLBACK(modest_ui_dimming_rules_on_view_next) },
	{ "/ToolBar/ToolbarCancel", NULL },
};

G_END_DECLS
#endif /* __MODEST_MSG_VIEW_WINDOW_UI_PRIV_H__ */
