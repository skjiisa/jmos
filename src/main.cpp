#include <iostream>
#include <fstream>
#include <ios>
#include <cstdlib>
#include <string>
#include <algorithm>
#include "json.hpp"
#include "ini.hpp"
#include "cxxopts.hpp"

using namespace std;
using json = nlohmann::json;

// These global variables get definied at file load in main() and are not modified later
//string game="null";
json gameList;

enum URLs { mod, image };

// Basically shorthand for getting commonly-used URLs
string url(URLs type, string game){
	stringstream s;
	s << gameList[game]["id"];

	switch(type){
		case mod:	return "https://www.nexusmods.com/" + game + "/mods/";
		case image :	return "https://staticdelivery.nexusmods.com/mods/" + s.str() +"/images/";
	}
	return "";
}

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

// The below functions are for printing sections of the document

//// Output a filtered list of all the mods from a given category
stringstream filterCategory(json& mods, string& game, string& category, char columns){
	stringstream output;

	if(category!="null"){
		stringstream line1, line2;
		char col = 0;

		output	<<"### Category: "<<category<<"\n\n";
		line1	<<"|";
		line2	<<"|";
		for(char i = 0; i < columns; i++){
			line1 <<"   |";
			line2 <<"---|";
		}
		output	<< line1.rdbuf() << "\n" << line2.rdbuf() << "\n";

		line1.str(string());
		line2.str(string());
		for(json::iterator it = mods.begin(); it != mods.end(); ++it){
			if( jsonListContains(it.value()["categories"], category) ){
				line1<<"| ["<<it.key()<<"](#"<<linkify(it.key())<<") ";
				line2<<"| ![]("<< p(it.value()["main image"]) <<") ";
				col++;
				if(col > columns - 1){
					col = 0;
					output	<< line1.rdbuf() << "\n" << line2.rdbuf() << "\n";
					line1.str(string());
					line2.str(string());
				}
			}
		}
		if(col > 0){
			output	<< line1.rdbuf() << "\n" << line2.rdbuf() << "\n";
		}
	}

	return output;
}

//// Output a masterlist of all mods
stringstream modMasterList(json& mods, string& game, json& categories, char columns){
	stringstream output;

	output <<"### Mod master list\n\n";
	for(json::iterator it = mods.begin(); it != mods.end(); ++it){
		stringstream line1, line2;
		char col = 0;

		output	<<"\n#### "<<it.key()<<"\n\n"

			<< p(it.value()["description"]) <<"\n\n";
		
		for(json::iterator jt = it.value()["id"].begin();
				jt != it.value()["id"].end();
				++jt){
			output	<<"["<< p(gameList[jt.key()]["name"]) <<" link]"
				<<"(" << p(url(mod, jt.key())) << jt.value() << ")\n\n";
		}

		line1	<<"| Images | ![]("<< p(it.value()["main image"]) <<") |";
		line2	<<"| ------ |:---:|";
		for(char i = 0; i < (columns - 2); i++){
			line1 <<"   |";
			line2 <<"---|";
		}

		output	<< line1.rdbuf() << "\n" << line2.rdbuf() << "\n";

		for(json::iterator jt = it.value()["images"].begin();
				jt != it.value()["images"].end();
				++jt){
			for(json::iterator kt = it.value()["images"][jt.key()].begin();
					kt != it.value()["images"][jt.key()].end();
					++kt, col = (col == columns - 1) ? 0 : col + 1 ){
				output<<"| ![]("<< p(url(image, jt.key())) << p(kt.value()) <<") ";
				if(col == columns - 1){
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

	return output;
}

//// List all the categories
stringstream categoryList(json& categories){
	stringstream output;

	output <<"\n## Categories\n\n";
	for(json::iterator it = categories.begin(); it != categories.end(); ++it){
		output<<"+ "<< p(it.value()) <<"\n";
	}

	return output;
}

// cxxopts command line argument parsing
cxxopts::ParseResult parse(int argc, char* argv[]){
	try{
		cxxopts::Options options(argv[0], " - example command line options");
		options.add_options()
			("c,category", "Category", cxxopts::value<std::string>())
			("g,game", "Game", cxxopts::value<std::string>())
			("h,help", "Print help")
		;

		options.parse_positional({"input", "output", "positional"});
		auto result = options.parse(argc, argv);

		if (result.count("help")){
			std::cout << options.help({""}) << std::endl;
			exit(0);
		}

		return result;
	}catch (const cxxopts::OptionException& e){
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}
}

int main(int argc, char* argv[]){
	json db, mods, categories;
	int unsigned jsize;
	char columns;
	ifstream ifile;
	ofstream ofile;
	stringstream output;
	json tmp;
	string category = "null", game = "null";

	auto result = parse(argc, argv);
	auto arguments = result.arguments();

	ifile.open("db.json");
	if(!ifile.is_open()){
		cout <<"Could not load database (db.json).\n";
		exit(1);
	}

	ifile >> db;
	ifile.close();
	ifile.clear();

	ifile.open("config.ini");
	if(!ifile.is_open()){
		cout <<"Could not load config.ini.\n";
		exit(1);
	}

	INI::Parser config(ifile);
	ifile.close();
	ifile.clear();

	ifile.open("gamelist.json");
	if(!ifile.is_open()){
		cout <<"Could not load game list (gamelist.json).\n";
		exit(1);
	}

	ifile >> gameList;
	ifile.close();
	ifile.clear();

	ofile.open("mods.md");
	if(!ofile.is_open()){
		cout <<"Could not load Markdown file.\n";
		exit(1);
	}

	if(config.top()["game"]==""){
		cout <<"A game needs to be specified in config.ini\n";
		exit(1);
	}else{
		game = config.top()["game"];
	}

	if(config.top()["category"]!=""){
		category = config.top()["category"];
	}

	if(result.count("category")){
		category = result["category"].as<std::string>();
	}

	cout<<"Sorting by category: "<<category<<"\n";

	columns = atoi( config.top()["columns"].c_str() );

	mods = db["Mods"];
	
	output	<<"# Skyrim\n\n"
		<<"## Mods\n\n";
	
	output << filterCategory(mods, game, category, columns).rdbuf();
	
	output << modMasterList(mods, game, categories, columns).rdbuf();

	output << categoryList(categories).rdbuf();

	ofile << output.rdbuf();

	cout<<"Markdown generation successful.\n";

	return 0;
}
