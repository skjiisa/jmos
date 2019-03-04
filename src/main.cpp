#include <iostream>
#include <fstream>
#include <ios>
#include <cstdlib>
#include <string>
#include <algorithm>
#include "json.hpp"
#include "ini.hpp"

using namespace std;
using json = nlohmann::json;

// Format a json string for printing
string p(json input){
	string text = input;
	text.erase(
		remove (text.begin(), text.end(), '\"'),
		text.end());
	return (text=="null")?"":text;
}

// Check if a json list contains a given string
bool jsonListContains(json list, string value){
	for(json::iterator it = list.begin(); it != list.end(); ++it){
		if(it.value() == value){
			return true;
		}
	}
	return false;
}

// Replaces every space in a sting with a - so it can be used as a header link in Markdown
string linkify(std::string text) {
    for(string::iterator it = text.begin(); it != text.end(); ++it) {
        if(*it == ' ') {
            *it = '-';
        }
    }
    return text;
}

int main(int argc, char* argv[]){
	if(argc > 2){
		cout<<"Invalid command-line arguments\n";
		exit(1);
	}

	json db, mods, categories, url;
	int unsigned jsize;
	ifstream ifile, inifile;
	ofstream ofile;
	stringstream output;
	json tmp;
	string category = "null";

	ifile.open("db.json");
	if(!ifile.is_open()){
		cout <<"Could not load database.\n";
		exit(1);
	}

	inifile.open("config.ini");
	if(!inifile.is_open()){
		cout <<"Could not load config ini.\n";
		exit(1);
	}

	ofile.open("mods.md");
	if(!ofile.is_open()){
		cout <<"Could not load Markdown file.\n";
		exit(1);
	}

	INI::Parser config(inifile);
	cout<<"category="<<config.top()["category"]<<"\n";
	if(config.top()["category"]!=""){
		category = config.top()["category"];
	}

	if(argc == 2){
		category = argv[1];
	}

	cout<<"Sorting by category: "<<category<<"\n";

	url["mod"] = "https://www.nexusmods.com/skyrim/mods/";
	url["image"] = "https://staticdelivery.nexusmods.com/mods/110/images/";

	ifile>>db;
	mods = db["Mods"];
	
	output	<<"# Skyrim\n\n"
		<<"## Mods\n\n";

	if(category!="null"){
		output <<"\n### Category: "<<category<<"\n\n";
		for(json::iterator it = mods.begin(); it != mods.end(); ++it){
			if( jsonListContains(it.value()["categories"], category) ){
				output<<"+ ["<<it.key()<<"](#"<<linkify(it.key())<<")\n";
			}
		}
	}

	output <<"### Mod master list\n\n";
	for(json::iterator it = mods.begin(); it != mods.end(); ++it){
		output	<<"\n#### "<<it.key()<<"\n\n"

			<< p(it.value()["description"]) <<"\n\n"
			<<"[Nexus link](" << p(url["mod"]) << it.value()["id"] << ")\n\n"

		// The below section should be altered to allow for dynamic table column numbers
			<<"| Images | ![]("<< p(url["image"]) << p(it.value()["main image"]) <<") |\n"
			<<"| ------ |:---:|\n";
		{	bool right = false;
			for(json::iterator jt = it.value()["images"].begin();
					jt != it.value()["images"].end();
					++jt, right=!right){
				output<<"| ![]("<< p(url["image"]) << p(jt.value()) <<")";
				if(right){
					output<<" |\n";
				}
			}
		}

		output<<"\n\nCategories:\n\n";
		for(json::iterator jt = it.value()["categories"].begin();
				jt != it.value()["categories"].end();
				++jt){
			output<<"+ "<<p(jt.value())<<"\n";
			if(!jsonListContains(categories, jt.value())){
				categories.push_back(jt.value());
			}
		}
	}

	output <<"\n### Categories\n\n";
	for(json::iterator it = categories.begin(); it != categories.end(); ++it){
		output<<"+ "<< p(it.value()) <<"\n";
	}

	ofile << output.rdbuf();

	cout<<"Markdown generation successful.\n";

	return 0;
}
