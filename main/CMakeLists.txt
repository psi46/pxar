# Build pXar test exe:
ADD_EXECUTABLE(testpxar "pxar.cpp" "pxar.h" )
TARGET_LINK_LIBRARIES(testpxar ${PROJECT_NAME})
