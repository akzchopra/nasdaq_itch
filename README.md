# NASDAQ ITCH File Processor

## Overview

The ITCH File Processor is a C++ program designed to process financial trading data stored in the ITCH format. The program reads large binary ITCH files, extracts trade information, calculates the Volume Weighted Average Price (VWAP) for each stock symbol per hour, and outputs the results to a text file.

## Features

Efficient File Processing: Uses memory mapping (mmap) to handle large ITCH files efficiently.
VWAP Calculation: Computes the VWAP and trade volume for each symbol on an hourly basis.
Error Handling: Robust error handling for file operations and parsing.

## Compile the Program
``
g++ -std=c++17 -O3 -pthread -lz main.cpp -o nasdaq
``


## Run the Program
``
./nasdaq path/to/your/itch_file.itch
``

#### NOTE : Run the program with the path to an ITCH file as an argument. This program assumes the ITCH file has been uncompressed.
#### Final output is printed out in a txt file with '_vwap_output' as suffix. It'll print the size of the file read, total trades that were processed and total time it took to process those trades.


## Code Structure

main.cpp: Contains the main logic for reading the ITCH file, processing trades, calculating VWAP, and outputting results.
Trade Structure: Represents individual trades with relevant fields like symbol, timestamp, price, and shares.
VWAP Structure: Holds VWAP value and trade volume for a given symbol and hour.


## Helper Functions:

- big_to_little_endian: Converts big-endian values to little-endian.
- timestamp_to_hour: Converts a timestamp to an hourly bucket.
- log_message: Logs messages to the console and output file.
- process_trade: Processes individual trades and updates VWAP calculations.
- parse_and_process_itch_file: Parses and processes the ITCH file using mmap.
- output_vwap: Outputs the VWAP calculations to the log file.

## Performance

Machine: MacBook Pro (2021)

Processor: Apple M1 Pro

Memory: 32 GB 

Output:

File size: 11245883092 bytes
Finished processing. Total messages: 368366634, Total trades processed: 9582065, Total time: 3 seconds