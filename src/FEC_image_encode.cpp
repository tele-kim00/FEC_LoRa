#include <iostream>
#include <fstream>                          // std::ifstream을 위한 라이브러리
#include <vector>
#include <string>                           // std::string을 위한 라이브러리
#include <iterator>                         // std::istreambuf_iterator
#include <RaptorQ/RaptorQ_v1_hdr.hpp>       // RaptorQ Library
#include <cstdio>
#include <cmath>

#include "base64.h"

int main()
{
    std::cout << "--- [TEST 1: CORRECT] Encoding(ID + Payload) to File  ---" << std::endl;

    // ==========================================================
    // A: File Setup & Read Original Data
    // ==========================================================
    const std::string output_filename = "../data/encoded_correct_image.txt";
    const std::string filename = "../data/sample_image.jpg";
    uint16_t symbol_size = 32;
    const double overhead_ratio = 10.0;

    // A-1: Read file in 'binary' mode
    std::ifstream file(filename, std::ios::binary);

    if(!file){
        std::cerr << "[ERROR] Cannot open original image file: " << filename << std::endl;
        return 1;
    }

    // A-2: Read entire file into vector (dynamic size)
    std::vector<uint8_t> source_data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    std::cout << " Total size: " << source_data.size() << " bytes" << std::endl;

    // ==========================================================
    // B: RaptorQ Encoder Setup
    // ==========================================================
    namespace RaptorQ = RaptorQ__v1;
    using namespace RaptorQ;

    using InputIt = std::vector<uint8_t>::iterator;
    using OutputIt = std::vector<uint8_t>::iterator;
    using Encoder = RaptorQ::Encoder<InputIt, OutputIt>;

    // B-1: Calculate minimum symbols needed
    uint32_t min_symbol = (source_data.size() + symbol_size - 1) / symbol_size;

    // B-2: Find valid Block_Size (most robust method)
    Block_Size block = Block_Size::Block_10;
    for(auto blk : *blocks) {
        if (static_cast<uint16_t>(blk) >= min_symbol){
            block = blk;
            break;
        }
    }
    // B-3: Actual source symbols (K) is the selected block size
    uint32_t num_source_symbols = static_cast<uint32_t>(block);

    std::cout << " Min symbols needed: " << min_symbol << std::endl;
    std::cout << " Selected Block Size(K): " << num_source_symbols << std::endl;

    Encoder encoder(block, symbol_size);

    // B-4: Set data to encoder and compute
    encoder.set_data(source_data.begin(), source_data.end());
    std::cout << "Computing symbols... " << std::endl;
    if (!encoder.compute_sync()){
        std::cerr << "Encoder pre-computation failed" << std::endl;
        return 1;
    }

    // ==========================================================
    // C: Symbol Generation & File Save (ID + Payload)
    // ==========================================================

    // C-1: Calculate repair symbols and total symbols
    uint32_t num_repair_symbols = static_cast<uint32_t>(ceil(num_source_symbols * (overhead_ratio / 100.0)));
    uint32_t total_symbols_to_send = num_source_symbols + num_repair_symbols;

    std::cout << " Repair symbols: " << num_repair_symbols << std::endl;
    std::cout << " Total symbols to send: " << total_symbols_to_send << std::endl;

    // C-2: Open output file for symbols
    std::ofstream output_file(output_filename);
    if (!output_file){
        std::cerr << "Error: Cannot open file" << output_filename << std::endl;
        return 1;
    }

    // C-3: Save source/repair symbols in a single loop
    std::cout << "Saving " << total_symbols_to_send << " (ID+Payload) packets to " << output_filename << "..." << std::endl;

    auto src_it = encoder.begin_source();
    auto repair_it = encoder.begin_repair();

    std::vector<uint8_t> payload(symbol_size); // 32-byte payload buffer
    std::vector<uint8_t> final_packet(4 + symbol_size); // 36-byte final packet buffer (pre-allocated)

    for (uint32_t i=0; i < total_symbols_to_send; ++i){

        uint32_t current_id;
        auto out_it = payload.begin();

        if(i < num_source_symbols){
                current_id = (*src_it).id();
                (*src_it)(out_it, payload.end());
                ++src_it;
        }
        else {
                current_id = (*repair_it).id();
                (*repair_it)(out_it, payload.end());
                ++repair_it;
        }

        // C-4: Combine [ID 4 bytes] + [Payload 32 bytes]
        final_packet[0] = (current_id >> 24) & 0xFF;
        final_packet[1] = (current_id >> 16) & 0xFF;
        final_packet[2] = (current_id >> 8) & 0xFF;
        final_packet[3] = current_id & 0xFF;
        std::copy(payload.begin(), payload.end(), final_packet.begin() + 4);

        // C-5: Encode to Base64 and write to file
        std::string base64_output = base64_encode(final_packet.data(), final_packet.size());
        output_file << base64_output << "\n";
    }

    output_file.close();
    std::cout << "[SUCCESS] File saved successfully. Total " << total_symbols_to_send << " symbols." << std::endl;

    return 0;
}
