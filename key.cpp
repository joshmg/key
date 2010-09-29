// File: key.cpp
// Written by Joshua Green

#include "shared/fileio/fileio.h"
#include "shared/db/db.h"
#include "shared/str/str.h"
#include <vector>
#include <map>
#include <string>
#include <iostream>
using namespace std;

const column TRANS_DATE_COL   = column("date", 30);
const column TRANS_TYPE_COL   = column("type", 30);
const column TRANS_DESC_COL   = column("description", 75);
const column TRANS_AMOUNT_COL = column("amount", 30);

const string TRANSACTION_MARKER = "<!-- end isLoc or isFdr if statement -->";
const string END_TRANSACTION_MARKER = "<!-- end for loop -->";

struct transaction {
  string date, type, description;
  float amount;

  row to_row() const {
    return row().add(TRANS_DATE_COL, date).add(TRANS_TYPE_COL, type).add(TRANS_DESC_COL, description).add(TRANS_AMOUNT_COL, ftos(amount));
  }
};

transaction get_transaction(fileio&);
string strip_whitespace(const string&);
string to_words(const string&, int words=3);

int main() {
  fileio data;
  database db;

  db.initialize("KeyBank");

  table* TRANSACTIONS = db.get_table("transactions");

  if (TRANSACTIONS == 0) {
    cout << "Creating transaction table...";
    TRANSACTIONS = db.add_table("transactions");
    TRANSACTIONS->add_column(TRANS_DATE_COL);
    TRANSACTIONS->add_column(TRANS_TYPE_COL);
    TRANSACTIONS->add_column(TRANS_DESC_COL);
    TRANSACTIONS->add_column(TRANS_AMOUNT_COL);
    cout << " done." << endl;
  }
  else cout << "Transaction table loaded." << endl;

  string input;
  cout << "Load new transactions? (y/n) ";
  getline(cin, input);
  if (input[0] == 'y' || input[0] == 'Y') {
    string filename;
    cout << "Filename: ";
    getline(cin, filename);

    data.open(filename, "r");
    if (!data.is_open()) {
      cout << "Unable to open \"" << filename << "\"." << endl;
      return 1;
    }

    data.read(-1, END_TRANSACTION_MARKER);
    int stop_pos = data.pos()-40;

    data.seek(0);
    data.read(-1, TRANSACTION_MARKER);

    cout << "Loading transactions...";
    while (data.pos() < stop_pos) {
      transaction t = get_transaction(data);
      TRANSACTIONS->add_row(t.to_row());
    }
    cout << " done." << endl << endl;
  }

  vector<row> results = TRANSACTIONS->select(query().where());
  /*cout << "Transactions:" << endl;
  for (int i=0;i<results.size();i++) {
    results[i].print(TRANSACTIONS);
  }*/

  float total_expenses = 0.0f, total_credits = 0.0f;
  map<string, float> totals;
  for (int i=0;i<results.size();i++) {
    if (totals.find(to_words(results[i][TRANS_DESC_COL])) == totals.end()) totals[to_words(results[i][TRANS_DESC_COL])] = 0.0f;
    totals[to_words(results[i][TRANS_DESC_COL])] += atof(results[i][TRANS_AMOUNT_COL].c_str());

    if (atof(results[i][TRANS_AMOUNT_COL].c_str()) < 0) total_expenses += atof(results[i][TRANS_AMOUNT_COL].c_str());
    else total_credits += atof(results[i][TRANS_AMOUNT_COL].c_str());
  }

  map<string, float>::iterator it = totals.begin();
  while (it != totals.end()) {
    for (int i=0;i<30;i++) {
      if (i < it->first.length()) cout << (it->first)[i];
      else cout << " ";
    }
    cout << ": ";
    if (it->second >= 0) cout << " ";
    cout << it->second << "\t(";

    if (it->second >= 0)  cout << floor((it->second/total_credits)*1000.0f + 0.5f)/10.0f << "%)" << endl;
    else                  cout << floor((it->second/total_expenses)*1000.0f + 0.5f)/10.0f << "%)" << endl;

    it++;
  }

  return 0;
}

transaction get_transaction(fileio& data) {
  transaction entry;
  string temp;

  data.read(-1, " >");
  entry.date = strip_whitespace(data.read(-1, "</td>"));
  data.read(-1, " >");
  entry.type = strip_whitespace(data.read(-1, "</td>"));
  data.read(-1, " >");
  entry.description = strip_whitespace(data.read(-1, "</td>"));

  data.read(-1, "align=\"right\">");
  temp = data.read(-1, "</td>");
  if (temp != "&nbsp;") {
    temp.insert(0, string("-"));
    entry.amount = atof(temp.c_str());
  }
  else {
    data.read(-1, "align=\"right\">");
    temp = data.read(-1, "</td>");
    entry.amount = atof(temp.c_str());
  }

  data.read(-1, "</tr>");

  return entry;
}

string strip_whitespace(const string& t) {
  string str = t;

  int del = 0;
  for (int i=0;i<str.length();i++) {
    if ( (str[i] == ' ' || str[i] == '\n' || str[i] == '\t' || str[i] == '\r')
        && (i+1 < str.length() && (str[i+1] == ' ' || str[i+1] == '\n' || str[i+1] == '\t' || str[i+1] == '\r'))
       ) del++;
    else {
      str.erase(i-del, del);
      i-= del;
      del = 0;
    }
  }
  
  for (int i=str.length()-1;i>0;i--) {
    if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') str.erase(i, 1);
    else break;
  }

  return str;
}

string to_words(const string& t, int words) {
  string str;

  int i=0;
  while (i < t.length()) {
    if (t[i] == ' ') words--;
    if (words > 0) {
      str += t[i];
    }
    else break;
    i++;
  }

  return str;
}
