add_executable(example_cygwin cygwin.c)
target_link_options(example_cygwin PRIVATE --entry mainCRTStartupHook)