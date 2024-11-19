#include <iostream>
#include <thread>
#include <mutex>
#include <fstream>
#include <string>
#include <sstream>
#include <cctype>
#include <vector>
#include <boost/asio.hpp>

using namespace std;

// constant strings
const string InputFileName = "Hamlet.txt";
const string horatioWord = "horatio";
const string horatioWordPossessive = "horatios";
const string andWord = "and";
const string hamletWord = "hamlet";
const string hamletWordPossessive = "hamlets";
const string godWord = "god";
const string godWordPossessive = "gods";

mutex allWordCountSem;
mutex horatioCountSem;
mutex andCountSem;
mutex hamletCountSem;
mutex godCountSem;

// cleans word of any symbols such as commas
string CleanWord(const string& word) {
    string cleaned;
    for (char letter : word)
        if (isalpha(letter))
            cleaned += letter;
    return cleaned;
}

// converts word to lowercase
string ToLower(const string& word) {
    string lower;
    for (char letter : word)
        lower += tolower(letter);
    return lower;
}

class Semaphore {
public:
    Semaphore(unsigned long init_count) {
        count = init_count;
    }

    void acquire() { // decrement the internal counter
        std::unique_lock<std::mutex> Lock(myMutex);
        while (!count) {
            Condition.wait(Lock);
        }
        count--;
    }

    void release() { // increment the internal counter
        std::unique_lock<std::mutex> Lock(myMutex);
        count++;
        Lock.unlock();
        Condition.notify_one();
    }

private:
    std::mutex myMutex;
    std::condition_variable Condition;
    unsigned long count;
};

// looks for elements to count in a line and increments them
void CountThroughLine(string& line, int& horatioCount, int& andCount, int& hamletCount, int& godCount, int& allWordCount) {

    stringstream stream(line);
    string word;

    // extract each word in the line
    while (stream >> word) {
        // clean symbols from words and make them lower case for interpretation
        string cleanedWord = ToLower(CleanWord(word));

        // if word isn't empty check if it is one of the words we're looking for
        if (!cleanedWord.empty()) {
            // check if word is one we're looking for
            if (cleanedWord == horatioWord || cleanedWord == horatioWordPossessive) {
                horatioCountSem.lock();
                horatioCount++;
                horatioCountSem.unlock();
            }
            else if (cleanedWord == andWord) {
                andCountSem.lock();
                ++andCount;
                andCountSem.unlock();
            }
            else if (cleanedWord == hamletWord || cleanedWord == hamletWordPossessive) {
                hamletCountSem.lock();
                hamletCount++;
                hamletCountSem.unlock();
            }
            else if (cleanedWord == godWord || cleanedWord == godWordPossessive) {
                godCountSem.lock();
                godCount++;
                godCountSem.unlock();
            }

            // increase word count
            allWordCountSem.lock();
            allWordCount++;
            allWordCountSem.unlock();
        }
    }
}

int main() {

    // open file
    ifstream inputFile(InputFileName);
    // check if file opened successfully
    if (!inputFile) {
        cerr << "could not open " << InputFileName << endl;
        return 1;
    }

    // establish words that will be counted
    int allWordCount = 0;
    int horatioCount = 0;
    int andCount = 0;
    int hamletCount = 0;
    int godCount = 0;

    // set semaphores for all variables
    //Semaphore allWordCountSem(1);
    //Semaphore horatioCountSem(1);
    //Semaphore andCountSem(1);
    //Semaphore hamletCountSem(1);
    //Semaphore godCountSem(1);

    // set the number of threads for boost pool to use
    boost::asio::thread_pool pool(thread::hardware_concurrency());

    // read through every line in parallel
    string line;
    while (getline(inputFile, line))
        boost::asio::post(pool, [&]() { CountThroughLine(line, horatioCount, andCount, hamletCount, godCount, allWordCount); });

    // join pool and close file
    pool.join();
    inputFile.close();

    // output results
    cout << horatioCount << " " << horatioWord << endl;
    cout << andCount << " " << andWord << endl;
    cout << hamletCount << " " << hamletWord << endl;
    cout << godCount << " " << godWord << endl;
    cout << "Total word count = " << allWordCount << endl;

    return 0;
}