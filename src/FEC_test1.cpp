#include <iostream>
#include <fstream>                          // std::ifstream을 위한 라이브러리
#include <vector>
#include <string>                           // std::string을 위한 라이브러리
#include <iterator>                         // std::istreambuf_iterator
#include <RaptorQ/RaptorQ_v1_hdr.hpp>       // RaptorQ Library
#include <cstdio>

void print_hex(const std::string& title, const std::vector<uint8_t>& data)
{
    std::cout << "--- " << title << " ---" << std::endl;
    std::cout << "Total Packet Size: " << data.size() << " Bytes" << std::endl;
    printf("HEX Data: ");
    for (size_t i = 0; i< data.size(); ++i){
        if (i == 4) {printf("| "); }
        if (i > 0 && i % 16 == 0 && i !=4) {printf("\n       ");}
        printf("%02x ", data[i]);
    }
    std::cout << std::endl << std::endl; 
}

int main()
{
    std::cout << "--- RaptorQ Test ---" << std::endl;

    // step1: File Input
    const std::string filename = "sample_data.txt";
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

    // Source Symbol 생성 및 출력
    std::cout<< "--- Source Symbols (first 5) ---" << std::endl;
    auto src_it = encoder.begin_source();
    for(uint32_t i=0; i < 5 && i < num_source_symbols; ++i, ++src_it){

        uint32_t current_id = (*src_it).id();
        std::vector<uint8_t> payload(symbol_size);
        auto out_it = payload.begin();
        (*src_it)(out_it, payload.end());

        uint8_t id_bytes[4];
        id_bytes[0] = (current_id >> 24) & 0xFF;
        id_bytes[1] = (current_id >> 16) & 0xFF;
        id_bytes[2] = (current_id >> 8) & 0xFF;
        id_bytes[3] = (current_id >> 0) & 0xFF;

        std::vector<uint8_t> final_packet;
        final_packet.insert(final_packet.end(), id_bytes, id_bytes + 4);
        final_packet.insert(final_packet.end(), payload.begin(), payload.end());

        print_hex("Source Symbol (ID)" + std::to_string(current_id) + ")", final_packet);
    }

    // Repair Symbol 생성 및 출력
    std::cout << "--- Repair Symbols (first 5) ---" << std::endl;
    auto repair_it = encoder.begin_repair();
    for (int i =0; i < 5; ++i, ++repair_it)
    {

        uint32_t current_id = (*repair_it).id();
        std::vector<uint8_t> payload(symbol_size);
        auto out_it = payload.begin();
        (*repair_it)(out_it, payload.end());

        uint8_t id_bytes[4];
        id_bytes[0] = (current_id >> 24) & 0xFF;
        id_bytes[1] = (current_id >> 16) & 0xFF;
        id_bytes[2] = (current_id >> 8) & 0xFF;
        id_bytes[3] = (current_id >> 0) & 0xFF;

        std::vector<uint8_t> final_packet;
        final_packet.insert(final_packet.end(), id_bytes, id_bytes + 4);
        final_packet.insert(final_packet.end(), payload.begin(), payload.end());

        print_hex("Repair Symbol (ID)" + std::to_string(current_id) + ")", final_packet);
    }


    return 0;
}
