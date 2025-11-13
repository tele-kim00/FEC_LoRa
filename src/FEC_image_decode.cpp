#include <iostream>
#include <fstream>      // ⬅️ std::ifstream, std::ofstream
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
    // ==========================================================
    // A: File Setup
    // ==========================================================
    if (argc != 2){
        std::cout << "[Error] Usage: ./decoder_image <input_file>" << std::endl;
        std::cout << "  Example: ./decoder_image ../data/encoded_image_correct.txt" << std::endl;
        return 1;
    }
    
    const std::string input_filename = argv[1];
    const std::string output_filename = "../data/decoded_image_result.jpg"; 
    const uint16_t symbol_size = 32;

    // A-1: (임시) 원본 파일 크기를 알아야 함
    //      (가장 좋은 방법은 이 값을 인코더에서 파일로 저장하고,
    //       디코더가 읽어오는 것이지만, 지금은 하드코딩합니다.)
    const uint32_t total_data_size = 1018; // (1018 바이트)

    std::cout << "--- " << input_filename << " File Decoding (Image)---" << std::endl;
    std::cout << "  Expecting original size: " << total_data_size << " bytes" << std::endl;


    // ==========================================================
    // B: RaptorQ Decoder Setup
    // ==========================================================
    namespace RaptorQ = RaptorQ__v1;
    using namespace RaptorQ;
    using InputIt = std::vector<uint8_t>::iterator;
    using OutputIt = std::vector<uint8_t>::iterator;
    using Decoder = RaptorQ::Decoder<InputIt,OutputIt>;

    // B-1: Calculate minimum symbols (Encoder와 동일한 로직)
    uint32_t min_symbol = (total_data_size + symbol_size - 1) / symbol_size;

    // B-2: Find valid Block_Size (Encoder와 동일한 로직)
    Block_Size block = Block_Size::Block_10; 
    for(auto blk : *blocks) {
        if (static_cast<uint16_t>(blk) >= min_symbol){
            block = blk;
            break;
        }
    }
    // B-3: Actual source symbols (K) (Encoder와 동일한 로직)
    uint32_t num_source_symbols = static_cast<uint32_t>(block);
    
    std::cout << "  Min symbols needed: " << min_symbol << std::endl;
    std::cout << "  Selected Block Size (K): " << num_source_symbols << std::endl;
    
    Decoder decoder(block, symbol_size, Decoder::Report::COMPLETE);

    // ==========================================================
    // C: Read File & Add Symbols
    // ==========================================================
    std::ifstream input_file(input_filename);
    if(!input_file){
        std::cerr << "Error: Cannnot open input File " << input_filename << std::endl;
        return 1; 
    }

    std::string line; // Base64 문자열 한 줄
    uint32_t received_count = 0;
    uint32_t line_number = 0;
    
    std::cout << "Reading packets from " << input_filename << "..." << std::endl;
    while (std::getline(input_file, line)){
        line_number++;
        
        try {
            std::string decoded_str = base64_decode(line);
            std::vector<uint8_t> received_packet(decoded_str.begin(), decoded_str.end());
            
            // C-1: Check packet size (ID + Payload)
            if (received_packet.size() == (4 + symbol_size)) {
                
                // C-2: Parse ID
                uint32_t symbol_id = (static_cast<uint32_t>(received_packet[0]) << 24) |
                                     (static_cast<uint32_t>(received_packet[1]) << 16) |
                                     (static_cast<uint32_t>(received_packet[2]) << 8)  |
                                     (static_cast<uint32_t>(received_packet[3]));
                
                // C-3: Get payload data (after 4 bytes)
                auto payload_start = received_packet.begin() + 4;
                
                // C-4: Add to decoder
                auto err = decoder.add_symbol(payload_start, received_packet.end(), symbol_id);

                if (err == RaptorQ::Error::NONE){
                    received_count++;
                } else if (err != RaptorQ::Error::NOT_NEEDED) {
                    std::cerr << "[Warning] Line " << line_number << ": Error adding symbol ID " << symbol_id << std::endl;
                }
            } 
            else {
                 std::cerr << "[Warning] Line " << line_number << ": Received packet with unexpected size (Size: " 
                           << received_packet.size() << "). Expecting 36 bytes. Ignoring." << std::endl;
            }
            
            // C-5: Check if ready
            if (decoder.ready()) {
                std::cout << ">>> Ready to decode after receiving " << received_count << " valid symbols." << std::endl;
                break;
            }

        } catch (const std::exception& e) {
            std::cerr << "[Warning] Line " << line_number << ": Base64 decode failed. Packet corrupted. (" << e.what() << ")" << std::endl;
        }
    }
    input_file.close();
    std::cout << "  Total valid symbols received: " << received_count << std::endl;

    // ==========================================================
    // D: Decode (wait_sync & decode_bytes)
    // ==========================================================
    if (decoder.ready()){
        std::cout << "Decoding (wait_sync)..." << std::endl;
        std::vector<uint8_t> decoded_data(total_data_size);
        auto out_it = decoded_data.begin();
        
        // D-1: Tell decoder no more symbols
        decoder.end_of_input(RaptorQ::Fill_With_Zeros::NO);
        // D-2: Run computation
        auto res = decoder.wait_sync();

        if (res.error != RaptorQ::Error::NONE){
            std::cerr << "[FAILURE] Decode failed during wait_sync(). Error code: " << static_cast<int>(res.error) << std::endl;
        }
        else {
            // D-3: Copy computed data to vector
            size_t decoded_from_byte = 0;
            size_t skip_bytes_at_begining_of_output = 0;
            auto decoded = decoder.decode_bytes(out_it, decoded_data.end(), 
                                                decoded_from_byte,
                                                skip_bytes_at_begining_of_output);
            
            // D-4: Check if size matches
            if (decoded.written == total_data_size) {
                // [Core] Write file in 'binary' mode
                std::ofstream out_file(output_filename, std::ios::binary);
                out_file.write(reinterpret_cast<const char*>(decoded_data.data()), decoded_data.size());
                out_file.close();

                std::cout << "[SUCCESS] Decode complete! Restored image saved to " << output_filename << std::endl;
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
