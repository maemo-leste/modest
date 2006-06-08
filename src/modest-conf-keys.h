#ifndef __MODEST_CONF_KEYS_H__
#define __MODEST_CONF_KEYS_H__

/* configuration key definitions for modest */
#define MODEST_CONF_NAMESPACE		"/apps/modest"

#define MODEST_CONF_USE_EXT_EDITOR	  MODEST_CONF_NAMESPACE "/use_ext_editor"	  /* boolean */
#define MODEST_CONF_EXT_EDITOR	          MODEST_CONF_NAMESPACE "/ext_editor"	  /* string */

#define MODEST_CONF_MAIN_WINDOW_HEIGHT	  MODEST_CONF_NAMESPACE "/main_window_height"    /* int */
#define MODEST_CONF_MAIN_WINDOW_HEIGHT_DEFAULT 480                                       /* int */

#define MODEST_CONF_MAIN_WINDOW_WIDTH	  MODEST_CONF_NAMESPACE "/main_window_width"     /* int */
#define MODEST_CONF_MAIN_WINDOW_WIDTH_DEFAULT  800

#define MODEST_CONF_EDIT_WINDOW_HEIGHT	  MODEST_CONF_NAMESPACE "/edit_window_height"    /* int */
#define MODEST_CONF_EDIT_WINDOW_HEIGHT_DEFAULT 480                                       /* int */

#define MODEST_CONF_EDIT_WINDOW_WIDTH	  MODEST_CONF_NAMESPACE "/edit_window_width"     /* int */
#define MODEST_CONF_EDIT_WINDOW_WIDTH_DEFAULT  800

#define MODEST_CONF_MSG_VIEW_NAMESPACE    MODEST_CONF_NAMESPACE "/view"

#define MODEST_CONF_MSG_VIEW_SHOW_ATTACHMENTS_INLINE MODEST_CONF_MSG_VIEW_NAMESPACE "/show_attachments_inline" /* boolean */


#endif /*__MODEST_CONF_KEYS_H__*/
