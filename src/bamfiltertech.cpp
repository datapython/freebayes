#include <iostream>
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <map>

#include "BamAlignment.h"
#include "BamReader.h"
#include "BamWriter.h"

#include "split.h"

using namespace std;
using namespace BamTools;

int main(int argc, char** argv) {

    if (argc == 1) {
        cerr << "usage: " << argv[0] << " [technology name] [ [technology name] ... ]" << endl;
        cerr << "filters BAM file piped on stdin for reads generated by a sequencing technology listed on the command line" << endl;
        return 1;
    }

    map<string, bool> technologies;

    for (int i = 1; i < argc; ++i) {
        technologies[argv[i]] = true;
    }

    BamReader reader;
    if (!reader.Open("stdin")) {
        cerr << "Could not open stdin for reading" << endl;
        return 1;
    }

    // retrieve header information
    map<string, string> readGroupToTechnology;

    string bamHeader = reader.GetHeaderText();

    vector<string> headerLines = split(bamHeader, '\n');

    for (vector<string>::const_iterator it = headerLines.begin(); it != headerLines.end(); ++it) {

        // get next line from header, skip if empty
        string headerLine = *it;
        if ( headerLine.empty() ) { continue; }

        // lines of the header look like:
        // "@RG     ID:-    SM:NA11832      CN:BCM  PL:454"
        //                     ^^^^^^^\ is our sample name
        if ( headerLine.find("@RG") == 0 ) {
            vector<string> readGroupParts = split(headerLine, "\t ");
            string tech;
            string readGroupID;
            for (vector<string>::const_iterator r = readGroupParts.begin(); r != readGroupParts.end(); ++r) {
                vector<string> nameParts = split(*r, ":");
                if (nameParts.at(0) == "PL") {
                   tech = nameParts.at(1);
                } else if (nameParts.at(0) == "ID") {
                   readGroupID = nameParts.at(1);
                }
            }
            if (tech.empty()) {
                cerr << " could not find PL: in @RG tag " << endl << headerLine << endl;
                return 1;
            }
            if (readGroupID.empty()) {
                cerr << " could not find ID: in @RG tag " << endl << headerLine << endl;
                return 1;
            }
            //string name = nameParts.back();
            //mergedHeader.append(1, '\n');
            //cerr << "found read group id " << readGroupID << " containing sample " << name << endl;
            readGroupToTechnology[readGroupID] = tech;
        }
    }

    // open writer, uncompressed BAM
    BamWriter writer;
    bool writeUncompressed = true;
    if ( !writer.Open("stdout", bamHeader, reader.GetReferenceData(), writeUncompressed) ) {
        cerr << "Could not open stdout for writing." << endl;
        return 1;
    }

    BamAlignment alignment;

    while (reader.GetNextAlignment(alignment)) {
        string name;
        if (alignment.GetTag("RG", name)) {
            if (technologies.find(readGroupToTechnology[name]) != technologies.end()) {
                //cout << name << " "  << readGroupToTechnology[name] << endl;
                writer.SaveAlignment(alignment);
            }
        }
    }

    reader.Close();
    writer.Close();

    return 0;

}
