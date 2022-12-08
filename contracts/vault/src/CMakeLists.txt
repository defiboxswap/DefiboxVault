project(vault)

find_package(cdt)

add_contract( vault vault vault.cpp )
target_include_directories( vault PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( vault ${CMAKE_SOURCE_DIR}/../ricardian )