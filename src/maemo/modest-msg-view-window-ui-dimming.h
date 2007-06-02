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
	{ "/MenuBar/MessageMenu/MessageDeleteMenu",  NULL },
	{ "/MenuBar/MessageMenu/MessageDetailsMenu", G_CALLBACK(modest_ui_dimming_rules_on_details) },

	/* Edit Menu */
	{ "/MenuBar/EditMenu", NULL },
	{ "/MenuBar/EditMenu/EditCutMenu", G_CALLBACK(modest_ui_dimming_rules_always_dimmed) },
	{ "/MenuBar/EditMenu/EditCopyMenu", NULL },
	{ "/MenuBar/EditMenu/EditPasteMenu", G_CALLBACK(modest_ui_dimming_rules_always_dimmed) },
	{ "/MenuBar/EditMenu/EditSelectAllMenu", NULL },
	{ "/MenuBar/EditMenu/EditMoveToMenu", G_CALLBACK(modest_ui_dimming_rules_on_move_to) },

	/* View Menu */
	{ "/MenuBar/ViewMenu", NULL },
	{ "/MenuBar/ViewMenu/ZoomMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewToggleFullscreenMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewPreviousMessageMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewNextMessageMenu", NULL },
	
	/* Attachments Menu */
	{ "/MenuBar/AttachmentsMenu", NULL },
	{ "/MenuBar/AttachmentsMenu/ViewAttachmentMenu", NULL },
	{ "/MenuBar/AttachmentsMenu/SaveAttachmentMenu", NULL },
	{ "/MenuBar/AttachmentsMenu/RemoveAttachmentMenu", NULL },

	/* Tools Menu */
	{ "/MenuBar/ToolsMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsContactsMenu", NULL },

	/* Close Menu */
	{ "/MenuBar/CloseMenu", NULL },
	{ "/MenuBar/ToolsMenu/CloseWindowMenu", NULL },
	{ "/MenuBar/ToolsMenu/CloseAllWindowsMenu", NULL },

	/* Contextual Menus (Toolbar) */
	{ "/ToolbarReplyCSM/MessageForwardMenu", NULL },
	{ "/ToolbarReplyCSM/MessageReplyAllMenu", NULL },
	{ "/ToolbarReplyCSM/MessageReplyMenu", NULL },
	
};

/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_view_toolbar_dimming_entries [] = {

	/* Toolbar */
	{ "/Toolbar/ToolbarMessageNew", NULL },
	{ "/Toolbar/ToolbarMessageReply", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/Toolbar/ToolbarDeleteMessage",  G_CALLBACK(modest_ui_dimming_rules_on_delete_msg) },
	{ "/Toolbar/ToolbarMoveTo", G_CALLBACK(modest_ui_dimming_rules_on_move_to) },
	{ "/Toolbar/ToolbarFindInMessage", NULL },
	{ "/Toolbar/ToolbarMessageBack", NULL },
	{ "/Toolbar/ToolbarMessageNext", NULL },
	{ "/Toolbar/ToolbarCancel", NULL },
};

G_END_DECLS
#endif /* __MODEST_MSG_VIEW_WINDOW_UI_PRIV_H__ */
