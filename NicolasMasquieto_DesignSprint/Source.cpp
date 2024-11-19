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
const string horatioWordPossessive = "horatios"; // accounts for possessive uses
const string andWord = "and";
const string hamletWord = "hamlet";
const string hamletWordPossessive = "hamlets"; // accounts for possessive uses
const string godWord = "god";
const string godWordPossessive = "gods"; // accounts for possessive uses

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

// looks for elements to count in a line and increments them
void CountThroughLine(const string& line, int& totalHoratioCount, int& totalAndCount, int& totalHamletCount, int& totalGodCount, int& totalWordCount, mutex& sharedCountsMutex) {

    // create scoped variables to count without much locking and unlocking
    int horatioCount = 0;
    int andCount = 0;
    int hamletCount = 0;
    int godCount = 0;
    int wordCount = 0;

    stringstream stream(line);
    string word;

    // extract each word in the line
    while (stream >> word) {
        // clean symbols from words and make them lower case for interpretation
        string cleanedWord = ToLower(CleanWord(word));

        // if word isn't empty check if it is one of the words we're looking for
        if (!cleanedWord.empty()) {
            // check if word is one we're looking for, increment if so
            if (cleanedWord == horatioWord || cleanedWord == horatioWordPossessive)
                horatioCount++;
            else if (cleanedWord == andWord)
                andCount++;
            else if (cleanedWord == hamletWord || cleanedWord == hamletWordPossessive)
                hamletCount++;
            else if (cleanedWord == godWord || cleanedWord == godWordPossessive)
                godCount++;

            // increase word count by default after checking
            wordCount++;
        }
    }

    // lock critical section where all variables are updated.
    // use a lock_guard which is an older version of scoped_lock, but is better with only 1 mutex
    scoped_lock lock(sharedCountsMutex);
    totalHoratioCount += horatioCount;
    totalAndCount += andCount;
    totalHamletCount += hamletCount;
    totalGodCount += godCount;
    totalWordCount += wordCount;
}

int main() {

    // open file
    ifstream inputFile(InputFileName);
    // check if file opened successfully
    if (!inputFile) {
        cerr << "could not open " << InputFileName << endl;
        return 1;
    }

    // establish counting variables
    int totalHoratioCount = 0;
    int totalAndCount = 0;
    int totalHamletCount = 0;
    int totalGodCount = 0;
    int totalWordCount = 0;

    // mutex that will be used to update counting variables
    mutex sharedCountsMutex;

    // set the number of threads for boost pool to use
    boost::asio::thread_pool pool(thread::hardware_concurrency());

    // read through every line in parallel
    string line;
    while (getline(inputFile, line)) {
        // Use boost thread pool to run CountThroughLine in each thread
        // variables are passed by reference
        boost::asio::post(pool, [line, &totalHoratioCount, &totalAndCount, &totalHamletCount, &totalGodCount, &totalWordCount, &sharedCountsMutex]() {
            CountThroughLine(line, totalHoratioCount, totalAndCount, totalHamletCount, totalGodCount, totalWordCount, sharedCountsMutex);
        });
    }

    // join pool and close file
    pool.join();
    inputFile.close();

    // output results
    std::cout << totalHoratioCount << " " << horatioWord << endl;
    std::cout << totalAndCount << " " << andWord << endl;
    std::cout << totalHamletCount << " " << hamletWord << endl;
    std::cout << totalGodCount << " " << godWord << endl;
    std::cout << "Total word count = " << totalWordCount << endl;

    return 0;
}