include_directories(
	../bouncer/src
	${Boost_INCLUDE_DIR}
)
add_library(log
	SHARED
	repo_log.cpp
	repo_broadcaster.cpp
)

target_link_libraries(log
	${Boost_LIBRARIES}
)

install(TARGETS "log" DESTINATION lib)