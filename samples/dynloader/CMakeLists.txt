# fully dynamic
add_library(dynloader SHARED dynloader.c)

# embeds the ezdotnet cli
add_library(loader SHARED loader.c)
target_link_libraries(loader ezdotnet_static)