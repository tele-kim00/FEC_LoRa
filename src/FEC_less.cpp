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
    std::cout << "--- [TEST 2: INCORRECT] Encoding(Payload only) ---" << std::endl;

    const std::string output_filename = "../data/encoded_incorrect.txt";
    
    // step1: File Input
    const std::string filename = "../data/sample_data.txt";
    std::ifstream file(filename, std::ios::binary);

    if(!file){
        std::cerr << "Error: Cannot open File " << filename << std::endl;
        return 1;
    }

    std::vector<uint8_t> source_data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    // Step2: RaptorQ Encoder

    uint16_t symbol_size = 32;
    const double overhead_ratio = 10.0;
    namespace RaptorQ = RaptorQ__v1;
    using namespace RaptorQ;

    // Encoder Templet에서 사용할 iterator type정의
    using InputIt = std::vector<uint8_t>::iterator;
    using OutputIt = std::vector<uint8_t>::iterator;
    using Encoder = RaptorQ::Encoder<InputIt, OutputIt>;

    // Block_Size Calculation
    uint32_t min_symbol = (source_data.size() + symbol_size - 1) / symbol_size;
    Block_Size block = Block_Size::Block_10;
    for(auto blk : *blocks) {
        if (static_cast<uint16_t>(blk) >= min_symbol){
            block = blk;
            break;
        }
    }
    uint32_t num_source_symbols = static_cast<uint32_t>(block);

    Encoder encoder(block, symbol_size);

    encoder.set_data(source_data.begin(), source_data.end());
    if (!encoder.compute_sync()){
        std::cerr << "Encoder pre-computation failed" << std::endl;
        return 1;
    }

    uint32_t num_repair_symbols = static_cast<uint32_t>(ceil(num_source_symbols * (overhead_ratio/100)));
    uint32_t total_symbols_to_send = num_source_symbols + num_repair_symbols;

    std::ofstream output_file(output_filename);
    if(!output_file){
        std::cerr << "Error: Cannot open file" << std::endl;
        return 1;
    }

    std::cout << "Saving" << total_symbols_to_send << " (Payload Only) packets to " << output_filename << "..." << std::endl;

    auto src_it = encoder.begin_source();
    auto repair_it = encoder.begin_repair();

    for (uint32_t i = 0; i < total_symbols_to_send; ++i){
        uint32_t current_id;

        std::vector<uint8_t> payload(symbol_size);
        auto out_it = payload.begin();

        if (i < num_source_symbols){
            current_id = (*src_it).id();
            (*src_it)(out_it, payload.end());
            ++src_it;
        }
        else{
            current_id = (*repair_it).id();
            (*repair_it)(out_it, payload.end());
            ++repair_it;
        }

        std::string base64_output = base64_encode(payload.data(), payload.size());

        output_file << base64_output << "\n";
    }

    output_file.close();
    std::cout << "File saved Sucessfully" << std::endl;

    return 0;
}

