project(win-monitor)

set(win-monitor_SOURCES
	win-monitor.c
    win-monitor.def
	win-monitor-plugin-main.c)

set(win-monitor_HEADERS
	win-monitor.h)

add_library(win-monitor MODULE
	${win-monitor_SOURCES}
	${win-monitor_HEADERS})

target_link_libraries(win-monitor
	libobs)
	
install_obs_plugin_with_data(win-monitor data)
