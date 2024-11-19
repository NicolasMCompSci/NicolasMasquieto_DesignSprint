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
void CountThroughLine(string& line, atomic<int>& horatioCount, atomic<int>& andCount, atomic<int>& hamletCount, atomic<int>& godCount, atomic<int>& allWordCount) {

    stringstream stream(line);
    string word;

    // extract each word in the line
    while (stream >> word) {
        // clean symbols from words and make them lower case for interpretation
        string cleanedWord = ToLower(CleanWord(word));

        // if word isn't empty check if it is one of the words we're looking for
        if (!cleanedWord.empty()) {
            // check if word is one we're looking for
            if (cleanedWord == horatioWord || cleanedWord == horatioWordPossessive)
                horatioCount.fetch_add(1);
            else if (cleanedWord == andWord)
                andCount.fetch_add(1);
            else if (cleanedWord == hamletWord || cleanedWord == hamletWordPossessive)
                hamletCount.fetch_add(1);
            else if (cleanedWord == godWord || cleanedWord == godWordPossessive)
                godCount.fetch_add(1);

            // increase word count
            allWordCount.fetch_add(1);
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
    int totalWordCount = 0;
    int totalHoratioCount = 0;
    int totalAndCount = 0;
    int totalHamletCount = 0;
    int totalGodCount = 0;

    // set the number of threads for boost pool to use
    boost::asio::thread_pool pool(thread::hardware_concurrency());

    // read through every line in parallel
    string line;
    while (getline(inputFile, line)) {
        atomic<int> horatioCount(0), andCount(0), hamletCount(0), godCount(0), wordCount(0);

        boost::asio::post(pool, [&]() { 
            CountThroughLine(line, horatioCount, andCount, hamletCount, godCount, wordCount);
        });

        totalHoratioCount += horatioCount;
        totalAndCount += andCount;
        totalHamletCount += hamletCount;
        totalGodCount += godCount;
        totalWordCount += wordCount;
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