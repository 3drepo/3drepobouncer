include_directories(
	../bouncer/src
	${Boost_INCLUDE_DIRS}
)
add_library(log
	SHARED
	repo_log.cpp
	repo_broadcaster.cpp
)

target_link_libraries(log
	Boost::log_setup
	Boost::log
)

install(TARGETS "log" DESTINATION lib)