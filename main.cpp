#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct Trade {
    std::string symbol;
    uint64_t timestamp;
    double price;
    uint32_t shares;
};

struct VWAP {
    double value{0.0};
    uint64_t volume{0};
};

std::unordered_map<std::string, std::vector<VWAP>> hourly_vwaps;
std::ofstream output_file;

template<typename T>
T big_to_little_endian(T value) {
    if constexpr (sizeof(T) == 2) return ntohs(value);
    if constexpr (sizeof(T) == 4) return ntohl(value);
    if constexpr (sizeof(T) == 8) return ((uint64_t)ntohl(value & 0xFFFFFFFF) << 32) | ntohl(value >> 32);
    return value;
}

uint32_t timestamp_to_hour(uint64_t timestamp) {
    return static_cast<uint32_t>((timestamp / 3600'000'000'000ULL) % 24);
}

void log_message(const std::string& message) {
    std::cout << message << std::endl;
    output_file << message << std::endl;
    output_file.flush();
}

void process_trade(const Trade& trade) {
    uint32_t hour = timestamp_to_hour(trade.timestamp);
    auto& vwaps = hourly_vwaps[trade.symbol];
    if (vwaps.size() <= hour) {
        vwaps.resize(hour + 1);
    }
    VWAP& vwap = vwaps[hour];
    double new_volume = vwap.volume + trade.shares;
    vwap.value = (vwap.value * vwap.volume + trade.price * trade.shares) / new_volume;
    vwap.volume = new_volume;
}

void parse_and_process_itch_file(const std::string& filename) {
    // Open file
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("Error opening file: " + filename);
    }

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        throw std::runtime_error("Error getting file size: " + filename);
    }
    size_t file_size = sb.st_size;

    log_message("File size: " + std::to_string(file_size) + " bytes");

    // Memory-map the file
    char* file_data = static_cast<char*>(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file_data == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Error mapping file: " + filename);
    }

    // Process the file
    uint64_t total_messages = 0;
    uint64_t trades_processed = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto last_print_time = start_time;

    size_t offset = 0;

    while (offset < file_size) {
        if (offset + sizeof(uint16_t) > file_size) break;

        uint16_t message_length = *reinterpret_cast<uint16_t*>(file_data + offset);
        message_length = big_to_little_endian(message_length);
        offset += sizeof(uint16_t);

        if (offset + message_length > file_size) break;

        uint8_t message_type = static_cast<uint8_t>(file_data[offset]);
        total_messages++;

        if (message_type == 'P' || message_type == 'E' || message_type == 'C') {
            uint64_t timestamp = big_to_little_endian(*reinterpret_cast<uint64_t*>(file_data + offset + 5));
            uint32_t shares = big_to_little_endian(*reinterpret_cast<uint32_t*>(file_data + offset + 20));
            char symbol[9] = {0};
            std::memcpy(symbol, file_data + offset + 24, 8);
            uint32_t price_raw = big_to_little_endian(*reinterpret_cast<uint32_t*>(file_data + offset + 32));
            double price = static_cast<double>(price_raw) / 10000.0;

            process_trade({std::string(symbol), timestamp, price, shares});
            trades_processed++;
        }

        offset += message_length;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    log_message("Finished processing. Total messages: " + std::to_string(total_messages) +
                ", Total trades processed: " + std::to_string(trades_processed) +
                ", Total time: " + std::to_string(duration) + " seconds\n");

    // Unmap and close
    munmap(file_data, file_size);
    close(fd);
}

void output_vwap() {
    std::stringstream ss;
    for (const auto& [symbol, vwaps] : hourly_vwaps) {
        ss << "Symbol: " << symbol << std::endl;
        for (size_t i = 0; i < vwaps.size(); ++i) {
            if (vwaps[i].volume > 0) {
                ss << "Hour " << i << ": VWAP = " << std::fixed << std::setprecision(4)
                   << vwaps[i].value
                   << ", Volume = " << vwaps[i].volume << std::endl;
            }
        }
        ss << std::endl;
    }
    log_message(ss.str());
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <itch_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::string output_filename = std::filesystem::path(filename).stem().string() + "_vwap_output.txt";
    output_file.open(output_filename);

    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file: " << output_filename << std::endl;
        return 1;
    }

    try {
        parse_and_process_itch_file(filename);
        output_vwap();

        output_file.close();
        std::cout << "Output has been written to " << output_filename << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        output_file << "An error occurred: " << e.what() << std::endl;
        output_file.close();
        return 1;
    }

    return 0;
}
