project(stoken)

find_package(cdt)

add_contract( stoken stoken stoken.cpp )
target_include_directories( stoken PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( stoken ${CMAKE_SOURCE_DIR}/../ricardian )