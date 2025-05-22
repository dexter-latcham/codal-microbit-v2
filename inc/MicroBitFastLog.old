#ifndef MICROBIT_FAST_LOG_H
#define MICROBIT_FAST_LOG_H

#include "CodalConfig.h"
#include "MicroBitLog.h"

namespace codal {

class MicroBitFastLog {
private:
    int columnCount;     // Number of columns per row
    int head;            // Index of next insert position (circular buffer)
    int count;           // Current number of rows stored
    int maxRows;         // Maximum rows that fit in allocated memory
    int status;          // 0 = uninitialized, 1 = initialized
    float* data;         // Pointer to raw data block

public:
    MicroBitFastLog(int numberColumns);

    /**
     * Initializes the data buffer.
     */
    void init();

    /**
     * Logs a new row of data. Overwrites oldest row if buffer is full.
     */
    void logRow(float* dataArray);

    /**
     * Retrieves a row of data by index.
     * @param returnArray A pre-allocated array to receive the row.
     * @param rowNumber Index of the row to retrieve (0 = oldest).
     */
    void getRow(float* returnArray, int rowNumber);

    /**
     * Gets the number of rows currently stored.
     */
    int getRowCount();
    void writeToDisk(codal::MicroBitLog &log);
};

} // namespace codal

#endif // MICROBIT_FAST_LOG_H
