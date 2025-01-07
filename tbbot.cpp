#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <random>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iomanip>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ext/pb_ds/assoc_container.hpp>
#include "httpev.cpp"
#include "dataset.cpp"
#pragma GCC optimize("Ofast,unroll-loops")
using namespace __gnu_pbds;
using namespace std;
using ll = long long;
using ull = unsigned long long;

const int MAXLISTEN = 768;
const int BUFF = 1024;

vector <string> quotes;
HTTPev httpev;
inline string genans(ll ind, Dataset &df, bool isData = false) {
    string data = df.readsomeline(ind);
    
    vector <string> parsed = df.parsestring(data); 
    string body = "<h1>" + parsed[1] + "</h1>\n<div class=\"images images-" + httpev.tprs(parsed[1]) + "\"></div>\n";
    body += "<h3><ul" + httpev.parselinetolist(parsed[2]) + "</ul></h3><ol" + httpev.parselinetolist(parsed[3]) + "</ol>"; 
	body += "<p><i>";
	for (int i = 6; i < parsed.size(); ++i) {
		body += "<a href=\"/" + parsed[i] + "/0\">";
		for (int j = 0; j < parsed[i].size(); ++j) {
			if (parsed[i][j] == '_') {
				parsed[i][j] = ' ';
			}
		}
		body += parsed[i] + "</a>";
		if (i + 1 < parsed.size()) {
			body += ", ";
		}
	}
	body += "</i></p>";
    string res;
	if (isData) {
		res	= "<div>\n" + body + "</div>";
	} else {
    	res = "<!DOCTYPE html><html>\n<head><meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\"><link rel=\"stylesheet\" id=\"theme\">" + httpev.javascriptswitchtheme + "\n<title>" + parsed[1] + "</title>\n</head>\n<body><script src=\"/JSIMAGE\"></script>\n<div>\n" + body + "<p>Taken from <a href=\"https://" + parsed[4] + "\">" + parsed[4] +"</a> (may not work)</p>" + "</div>\n<h3 style=\"margin: 1em\" class=\"button\"><a href=\"/\">Another recipe</a></h3>\n<h3 align=\"center\" class=\"button\"><a onclick=\"switchTheme()\">Switch theme</a></h3></body><script>text=document.getElementsByTagName(\"body\")[0];text.innerHTML=text.innerHTML.replaceAll(\"\\\\n\",\"\\n\").replaceAll(\"\\\\r\",\"\\r\").replaceAll(\"\\\\t\",\"\\t\").replaceAll(\"\\\\u00b0\",\"\\u00b0\").replaceAll(\"u0b0\",\"\\u00b0\");getimage(\"" + httpev.tprs_name(parsed[1]) + "\");</script>\n</html>";
	}
    return httpev.genfromhtmltohttp(res);
}

inline string getindexes(string ing, ll pagenum, Dataset &df) {
    vector <pair<string, ll>> page = df.readpageindexes(ing, pagenum);
	string ingbetter = ing;
	for (int i = 0; i < ingbetter.size(); ++i) {
		if (ingbetter[i] == '_') {
			ingbetter[i] = ' ';
		}
	}
    string body = "<div><h1>" + ingbetter + "</h1>";
	body += "<div class=\"images images-" + httpev.tprs(ing) + "\"></div>";
    body += "<h3><ol start=" + httpev.lltos(pagenum * PAGESIZE + 1) +">";
    for (int i = 0; i < page.size(); ++i) {
        body += "<li><a href=\"/" + httpev.lltos(page[i].second) + "\">" + page[i].first + "</a></li>\n";
    }
    body += "</ol></h3></div>";
    body += "<table width=\"100%\"><tr>";
    if (pagenum > 0) {
        body += "<td><h2 style=\"margin: 1em\" align=\"left\" class=\"button\"><a href=\"/" + ing + "/" + httpev.lltos(pagenum - 1) + "\">Back</a></h2></td>";
    }
    if (pagenum < df.getlastpage(ing)) {
        body += "<td><h2 style=\"margin: 1em\" align=\"right\" class=\"button\"><a href=\"/" + ing + "/" + httpev.lltos(pagenum + 1) + "\">Forward</a></h2></td>";
    }
    body += "</tr></table>";
    string res = "<!DOCTYPE html><html>\n<head><meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\"><link rel=\"stylesheet\" id=\"theme\">" + httpev.javascriptswitchtheme +"\n<title>" + ingbetter + "</title>\n</head>\n<body><script src=\"/JSIMAGE\"></script>\n" + body + "\n<h3 align=\"center\" class=\"button\"><a onclick=\"switchTheme()\">Switch theme</a></h3><script>getimage(\"" + httpev.tprs_name(ing) + "\")</script></body>\n</html>";
    return httpev.genfromhtmltohttp(res);
}

inline string getindexessearch(string name, ll lastnum, Dataset &df) {
	string namebetter = name;
	for (int i = 0; i < namebetter.size(); ++i) {
		if (namebetter[i] == '_') {
			namebetter[i] = ' ';
		}
	}
    auto [page, nextnum] = df.find_search(namebetter, lastnum);
    string body = "<div><h1 class=\"button\"><input id=\"search\" type=\"search\" value=\"" + namebetter +"\" onkeypress=\"searchthis(event)\" placeholder=\"bread\"><a style=\"margin: 0.5em;\" onclick=\"searchthis(true)\">search</a></input></h1>";
	body += "<div class=\"images images-" + httpev.tprs(name) + "\"></div>";
	if (df.findind(name)) {
    	body += "<h3><a href=\"/" + name + "\">Ingredient: " + namebetter + "</a></h3>\n";
	}
    body += "<h3><ul>";
    for (int i = 0; i < page.size(); ++i) {
        body += "<li><a href=\"/" + httpev.lltos(page[i].second) + "\">" + page[i].first + "</a></li>\n";
    }
	if (page.size() == 0) {
		body += "<li>Nothing found</li>";
	}
    body += "</ul></h3></div>";
    body += "<table width=\"100%\"><tr>";
    if (lastnum >= 0) {
        body += "<td><h2 style=\"margin: 1em\" align=\"left\" class=\"button\"><a href=\"/SEARCH/" + name + "/\">Begin</a></h2></td>";
    }
	if (nextnum != -1) {
    	body += "<td><h2 style=\"margin: 1em\" align=\"right\" class=\"button\"><a href=\"/SEARCH/" + name + "/" + httpev.lltos(nextnum) + "\">Forward</a></h2></td>";
	} else {
    	body += "<td><h2 style=\"margin: 1em\" align=\"right\" class=\"button\"><a href=\"/\">Back to roll page</a></h2></td>";
	}
	body += "</tr></table>";
    string res = "<!DOCTYPE html><html>\n<head><meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\"><link rel=\"stylesheet\" id=\"theme\">" + httpev.javascriptswitchtheme +"\n<title>" + namebetter + "</title>\n</head>\n<body><script src=\"/JSIMAGE\"></script>\n" + body + "\n<h3 align=\"center\" class=\"button\"><a onclick=\"switchTheme()\">Switch theme</a></h3><script>getimage(\"" + httpev.tprs_name(name) + "\")</script></body>\n</html>";
    return httpev.genfromhtmltohttp(res);
}



inline void servingclient(int sock, Dataset &df) {
	mt19937_64 randomizer = mt19937_64(chrono::system_clock::now().time_since_epoch().count());
    string ans;
    char recved[BUFF];
	int recvedlen = 0;
    while (true) {
		recvedlen = recv(sock, recved + recvedlen, BUFF - recvedlen, 0);
        if (recvedlen > 0) {
            ll cn = -1;
			vector <string> res(1);
			for (int i = 0; i < recvedlen; ++i) {
				if (recved[i] == '/') {
					res.push_back("");
				} else if (recved[i] != ' ') {
					res.back().push_back(recved[i]);
				} else {
					if (res.size() > 1) {
						break;
					}
				}
			}
			if (res.size() == 0) {
				close(sock);
				break;
			} else if (res[0] != "GET") {
				continue;
			} else {
				recvedlen = 0;
			}

			if (res.size() > 1) {
				cn = httpev.stoll(res[1]);
			}
            if (cn == -1 || cn < 0 || cn >= df.filedump_size) {
				if (res.size() > 1 && df.findind(res[1]) && res[1].size() > 0) {
                    if (res.size() > 2) {
			    	    cn = httpev.stoll(res[2]);
                        if (cn == -1 || cn < 0) {
                            ans = httpev.genindexingredtohttp(res[1]);
                        } else if (cn > df.getlastpage(res[1])) {
                            ans = httpev.genindexingredlasttohttp(res[1], df.getlastpage(res[1]));
                        } else {
                            ans = getindexes(res[1], cn, df);
                        }
                    } else {
                        ans = httpev.genindexingredtohttp(res[1]);
                    }
                } else if (res.size() > 1 && res[1] == "SEARCH") {
                    if (res.size() > 2 && res[2].size() > 0) {
						if (res.size() > 3) {
                        	ans = getindexessearch(res[2], httpev.stoll(res[3]), df);
						} else {
                        	ans = getindexessearch(res[2], -1, df);
						}
					} else {
                    	ans = httpev.genindextohttp(randomizer() % df.filedump_size);
					}
                } else if (res.size() > 1 && res[1] == "ROLL") {
					ans = httpev.htmlrolltohttp();
                } else if (res.size() > 2 && res[1] == "CSS") {
					ans = httpev.csstohttp(res[2]);
                } else if (res.size() > 1 && res[1] == "JSIMAGE") {
					ans = httpev.jsimagetohttp();
				} else if (res.size() <= 1 || res[1].size() == 0) {
                    if (randomizer() % 4 == 0) {
                        ans = httpev.genrolltohttp(randomizer() % df.filedump_size, quotes[randomizer() % quotes.size()]);
                    } else {
                        ans = httpev.genrolltohttp(randomizer() % df.filedump_size, "");
                    }
				} else {
                    ans = httpev.genindextohttp(randomizer() % df.filedump_size);
                }
            } else {
				if (res.size() > 2 && res[2] == "DATA") {
                	ans = genans(cn, df, true);
                } else {
					ans = genans(cn, df, false);
				}
            }
            send(sock, ans.data(), ans.size(), 0);
        } else {
            close(sock);
            break;
        }
    }
}

inline void server(Dataset &df, int port) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        cerr << "Error socket" << endl;
        exit(1);
    }
    const int enable = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        cerr << "Error setsockopt" << endl;
        exit(1);
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (sockaddr *)&addr, sizeof(addr)) < 0) {
        cerr << "Error bind" << endl;
        exit(1);
    }

    listen(listener, MAXLISTEN);
    cout << "Listening" << endl;
    while (true) {
        int sock = accept(listener, NULL, NULL);
        if (sock < 0) {
            #ifdef DEBUG
            cerr << "Error accept" << endl;
            #endif
        } else {
            auto thr = thread(servingclient, sock, ref(df));
            thr.detach();
        }
    }
}

void quotesload(string filename) {
	ifstream fin(filename);
	string line;
	while (!fin.eof()) {
		getline(fin, line);
		if (line.back() == '\n') {
			line.pop_back();
		}
		quotes.push_back(line);
	}
	if (quotes.size() == 0) {
		quotes.push_back("");
	} else {
		cout << "Quotes loaded" << endl;
	}
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(0);
    cout.tie(0);
    cout << fixed << setprecision(3);
    Dataset df("full_dataset.csv");
	quotesload("quotes.txt");
    server(df, 501);
}

