#ifndef _SYS_CYGWIN_H
#define _SYS_CYGWIN_H

#if defined(_MSC_VER)
typedef SSIZE_T ssize_t;
#endif

/* Possible 'what' values in calls to cygwin_conv_path/cygwin_create_path. */
enum
{
  CCP_POSIX_TO_WIN_A = 0, /* from is char*, to is char*       */
  CCP_POSIX_TO_WIN_W,	  /* from is char*, to is wchar_t*    */
  CCP_WIN_A_TO_POSIX,	  /* from is char*, to is char*       */
  CCP_WIN_W_TO_POSIX,	  /* from is wchar_t*, to is char*    */

  CCP_CONVTYPE_MASK = 3,

  /* Or these values to the above as needed. */
  CCP_ABSOLUTE = 0,	  /* Request absolute path (default). */
  CCP_RELATIVE = 0x100    /* Request to keep path relative.   */
};
typedef unsigned int cygwin_conv_path_t;

#endif