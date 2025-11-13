#include <iostream>
#include <fstream>      // ⬅️ 파일 읽기 (ifstream), 파일 쓰기 (ofstream)
#include <vector>
#include <string>
#include <iterator>
#include <RaptorQ/RaptorQ_v1_hdr.hpp> // RaptorQ Library
#include <cstdio>
#include <cmath>
#include <stdexcept>    // ⬅️ Base64 에러 처리를 위해 추가

// ⬅️ Base64 디코딩을 위해 헤더 포함
#include "base64.h"

int main(int argc, char* argv[])
{
    // Step1: Data Input
    if (argc != 2){
        std::cout << "[Error] 사용법 오류: ./FEC_decoder_no_id <input_file>" << std::endl;
        return 1;
    }

    
    const std::string input_filename = argv[1];
    // ID가 없는 버전용 출력 파일
    const std::string output_filename = "../data/decoded_incorrect.txt";

    std::cout << "--- " << input_filename << " File Decoding (ID-less)---" << std::endl;

    // Step2: 메타데이터 설정 (ID가 있는 버전과 동일)
    const uint16_t symbol_size = 32;
    const uint32_t total_data_size = 660;
    const uint32_t num_source_symbols = 26;

    // Step3: Decoder 설정 (ID가 있는 버전과 동일)
    namespace RaptorQ = RaptorQ__v1;
    using namespace RaptorQ;
    using InputIt = std::vector<uint8_t>::iterator;
    using OutputIt = std::vector<uint8_t>::iterator;
    using Decoder = RaptorQ::Decoder<InputIt,OutputIt>;

    Block_Size block = static_cast<Block_Size>(num_source_symbols);
    Decoder decoder(block, symbol_size, Decoder::Report::COMPLETE);

    std::ifstream input_file(input_filename);
    if(!input_file){
        std::cerr << "Error: Cannnot open input File " << input_filename << std::endl;
        return 1; 
    }

    std::string line; // Base64 문자열 한 줄
    uint32_t received_count = 0;
    uint32_t line_number = 0;
    // Text File 한 줄씩 읽기
    while (std::getline(input_file, line)){
        line_number++;
        
        try {
            std::string decoded_str = base64_decode(line);
            std::vector<uint8_t> received_packet(decoded_str.begin(), decoded_str.end());
            
            // --- C. [핵심] ID가 없는 패킷(32바이트)만 처리 ---
            
            // 패킷 크기가 순수 심볼 32바이트인지 확인
            if (received_packet.size() == symbol_size) {
                
                // [변경] ID를 파일 줄 번호로 "가정" (ESI는 0부터 시작)
                uint32_t assumed_symbol_id = line_number - 1;
                
                // [변경] 페이로드 시작 위치 = 패킷의 시작 (0번 인덱스)
                auto payload_start = received_packet.begin();
                
                auto err = decoder.add_symbol(payload_start, received_packet.end(), assumed_symbol_id);

                if (err == RaptorQ::Error::NONE){
                    received_count++;
                    // [변경] 로그 메시지 수정
                    std::cout << " -> Added symbol with assumed ID: " << assumed_symbol_id << " (Total vaild: " << received_count << " )" << std::endl;
                }else if (err != RaptorQ::Error::NOT_NEEDED) {
                    std::cerr << "[Warning] Line " << line_number << ": Error adding symbol with assumed ID " << assumed_symbol_id << std::endl;
                }
            } 

            // 그 외 (ID가 있거나(36) 손상된 패킷(기타), 무시)
            else {
                 // [변경] 기대하는 바이트 크기 수정
                 std::cerr << "[Warning] Line " << line_number << ": Received packet with unexpected size (Size: " 
                           << received_packet.size() << "). Expecting 32 bytes. Ignoring." << std::endl;
            }
            
            if (decoder.ready()) {
                std::cout << ">>> Ready to decode after receiving " << received_count << " valid symbols." << std::endl;
                break;
            }

        } catch (const std::exception& e) {
            std::cerr << "[Warning] Line " << line_number << ": Base64 decode failed. Packet corrupted. (" << e.what() << ")" << std::endl;
        }
    }
    input_file.close();

    // --- Step 4: 디코딩 시도 (ID가 있는 버전과 100% 동일) ---
    if (decoder.ready()){
        std::cout << "Decoding... " <<std::endl;
        std::vector<uint8_t> decoded_data(total_data_size);
        auto out_it = decoded_data.begin();
        
        // 더 이상 입력이 없음을 알림
        decoder.end_of_input(RaptorQ::Fill_With_Zeros::NO);

        // 디코딩 계산을 동기식으로 실행
        auto res = decoder.wait_sync();

        if (res.error != RaptorQ::Error::NONE){
            std::cerr << "[FAILURE] Decode failed during wait_sync(). Error code: " << static_cast<int>(res.error) << std::endl;
        }
        else {
            // 계산된 데이터를 벡터로 추출 (decoded_bytes 사용)
            size_t decoded_from_byte = 0;
            size_t skip_bytes_at_begining_of_output = 0;
            auto decoded = decoder.decode_bytes(out_it, decoded_data.end(), 
                                                decoded_from_byte,
                                                skip_bytes_at_begining_of_output);
            
            if (decoded.written == total_data_size) {
                // Success
                std::ofstream out_file(output_filename);
                out_file.write(reinterpret_cast<const char*>(decoded_data.data()), decoded_data.size());
                out_file.close();

                std::cout << "[SUCCESS] Decode complete! Restored data saved to " << output_filename << std::endl;
            } else {
                std::cerr << "[FAILURE] Decode failed. Wrote " << decoded.written << " bytes, expected " << total_data_size << std::endl;
            }
        }
    } else {
        std::cerr << "[FAILURE] Decode failed. Not enough valid symbols received." << std::endl;
        std::cerr << "  (Received " << received_count << " valid symbols, needed " << num_source_symbols << ")" << std::endl;
    }

    return 0;
}
