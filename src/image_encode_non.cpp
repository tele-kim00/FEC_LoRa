#include <iostream>
#include <fstream>      // ⬅️ std::ifstream, std::ofstream
#include <vector>
#include <string>
#include <iterator>     // std::istreambuf_iterator
#include <RaptorQ/RaptorQ_v1_hdr.hpp> // RaptorQ Library
#include <cstdio>
#include <cmath>        // ceil()
#include <stdexcept>

// ⬅️ Include Base64 header
#include "base64.h" 

// --- Image File Encoder (ID-less version) ---
int main()
{
    std::cout << "--- Starting encode for image (ID-less version) ---" << std::endl;

    // ==========================================================
    // A: File Setup & Read Original Data
    // ==========================================================
    const std::string original_filename = "../data/sample_image.jpg"; 
    // [변경] ID가 없는 버전용 출력 파일
    const std::string output_filename = "../data/encoded_image_no_id.txt";
    const uint16_t symbol_size = 32; 
    const double overhead_ratio = 10.0; // 30% overhead

    // A-1: Read file in 'binary' mode
    std::ifstream input_file(original_filename, std::ios::binary);
    if (!input_file) {
        std::cerr << "[ERROR] Cannot open original image file: " << original_filename << std::endl;
        return 1;
    }
    
    // A-2: Read entire file into vector (dynamic size)
    std::vector<uint8_t> original_data(
        (std::istreambuf_iterator<char>(input_file)),
        std::istreambuf_iterator<char>()
    );
    input_file.close();

    std::cout << "  Total size: " << original_data.size() << " bytes" << std::endl;

    // ==========================================================
    // B: RaptorQ Encoder Setup (ID가 있는 버전과 100% 동일)
    // ==========================================================
    namespace RaptorQ = RaptorQ__v1;
    using namespace RaptorQ;
    
    using InputIt = std::vector<uint8_t>::iterator;
    using OutputIt = std::vector<uint8_t>::iterator;
    using Encoder = RaptorQ::Encoder<InputIt, OutputIt>;

    // B-1: Calculate minimum symbols needed
    uint32_t min_symbol = (original_data.size() + symbol_size - 1) / symbol_size;

    // B-2: Find valid Block_Size (most robust method)
    Block_Size block = Block_Size::Block_10; // default
    for(auto blk : *blocks) {
        if (static_cast<uint16_t>(blk) >= min_symbol){
            block = blk; // first valid block size >= min_symbol
            break;
        }
    }
    // B-3: Actual source symbols (K) is the selected block size
    uint32_t num_source_symbols = static_cast<uint32_t>(block);
    
    std::cout << "  Min symbols needed: " << min_symbol << std::endl;
    std::cout << "  Selected Block Size (K): " << num_source_symbols << std::endl;

    Encoder encoder(block, symbol_size);

    // B-4: Set data to encoder and compute
    encoder.set_data(original_data.begin(), original_data.end());
    std::cout << "Computing symbols..." << std::endl;
    if (!encoder.compute_sync()) {
        std::cerr << "[ERROR] Encoder compute failed!" << std::endl;
        return 1;
    }

    // ==========================================================
    // C: Symbol Generation & File Save (Payload Only)
    // ==========================================================

    // C-1: Calculate repair symbols and total symbols
    uint32_t num_repair_symbols = static_cast<uint32_t>(ceil(num_source_symbols * (overhead_ratio / 100.0)));
    uint32_t total_symbols_to_send = num_source_symbols + num_repair_symbols;

    std::cout << "  Repair symbols: " << num_repair_symbols << std::endl;
    std::cout << "  Total symbols to send: " << total_symbols_to_send << std::endl;

    // C-2: Open output file for symbols
    std::ofstream output_file(output_filename);
    if (!output_file) {
        std::cerr << "[ERROR] Cannot open output file: " << output_filename << std::endl;
        return 1;
    }
    
    // C-3: Save source/repair symbols in a single loop
    std::cout << "Saving " << total_symbols_to_send << " (Payload Only) packets to " << output_filename << "..." << std::endl;

    auto src_it = encoder.begin_source();
    auto repair_it = encoder.begin_repair();

    std::vector<uint8_t> payload(symbol_size); // 32-byte payload buffer
    // [변경] final_packet 버퍼가 필요 없음

    for (uint32_t i = 0; i < total_symbols_to_send; ++i) {

        // [변경] current_id가 필요 없음
        auto out_it = payload.begin();

        if (i < num_source_symbols) {
            // (*src_it).id(); // ID는 사용하지 않음
            (*src_it)(out_it, payload.end());
            ++src_it;
        } else {
            // (*repair_it).id(); // ID는 사용하지 않음
            (*repair_it)(out_it, payload.end());
            ++repair_it;
        }

        // [변경] C-4: ID 결합 로직 (final_packet) 제거
        
        // C-5: [변경] 32바이트 페이로드(payload)를 직접 Base64 인코딩
        std::string base64_output = base64_encode(payload.data(), payload.size());
        output_file << base64_output << "\n";
    }

    output_file.close();
    std::cout << "[SUCCESS] File saved successfully. Total " << total_symbols_to_send << " symbols." << std::endl;

    return 0;
}
