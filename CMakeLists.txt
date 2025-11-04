# ===================================================================
# 1. 기본 프로젝트 설정
# ===================================================================

# 최소 CMake 버전 요구 (표준)
cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 설정 (IDE에 "FEC_LoRa"로 표시됨)
project(FEC_LoRa LANGUAGES CXX)

# C++11 표준을 사용하도록 설정 (g++의 -std=c++11 옵션과 동일)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# ===================================================================
# 2. 경로 설정 (헤더 & 라이브러리)
# ===================================================================

# 1. 우리 프로젝트의 헤더 파일 경로 (include/)
#    (-I../include 와 동일)
include_directories(include)

# 2. 외부 라이브러리 헤더 경로 (RaptorQ, Eigen)
#    (-I/usr/local/include 와 -I/usr/include/eigen3)
include_directories(/usr/local/include)
include_directories(/usr/include/eigen3)

# 3. 외부 라이브러리 .so 파일 경로 (RaptorQ)
#    (-L/home/pi1/libRaptorQ/build/lib)
#    (참고: pi1 유저 이름이 맞는지 확인하세요. /home/kim/ 이었다면 수정 필요)
link_directories(/home/pi1/libRaptorQ/build/lib) 


# ===================================================================
# 3. 소스 파일 정의 (가장 중요한 부분)
# ===================================================================

# [정의 1] "공용" 소스 파일 목록
# main() 함수가 "없는" 모든 .cpp 파일들을 여기에 추가합니다.
set(SHARED_SOURCES
    src/LoRaModule.cpp
    src/SerialPort.cpp
    src/base64.cpp
    # (나중에 디코더를 만들면 src/DecoderUtil.cpp 등을 추가)
)

# ===================================================================
# -----------------------FEC_Data Analysis---------------------------
#
# add_executable(FEC_analysis
#     src/FEC_test1.cpp
#     ${SHARED_SOURCES}
# )
#
# 수정하면 FEC_analysis Link 필요
# ===================================================================


# ===================================================================
# --------------------FEC-Base64_Data Analysis-----------------------
#
add_executable(FEC_base64
    src/FEC_base64.cpp
    ${SHARED_SOURCES}
)
#
#
# ===================================================================




# ===================================================================
# 4. 라이브러리 링크
# ===================================================================

# "base64_analysis"와 "lora_sender" 두 프로그램 모두에게
# libRaptorQ 라이브러리를 링크합니다. (-lRaptorQ, 대소문자 주의)
target_link_libraries(FEC_base64
    RaptorQ
    pthread
)
