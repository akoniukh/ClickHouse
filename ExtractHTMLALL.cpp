#include <vector>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

struct TagBlock {
    std::string text;
    int startpos, endpos;
};

struct SubSelector {
    char type;
    std::string value;
    std::string typeval;
};

bool insideComment(string html_section, int pos) {
    while (html_section[pos] != '<') {
        --pos;
    }
    return html_section.substr(pos, 4) == "<!--";
}

int FindTagStart(string html_section, int pos) {
    while (html_section[pos] != '<') {
        --pos;
    }
    return pos;
}

int FindTagEnd(string html_section, string name) {
    size_t pos = 0;
    int k = 0;
    while (true) {
        pos = html_section.find(name, pos + 1);
        if (pos == std::string::npos) {
            break;
        }
        if (insideComment(html_section, pos)) {
            continue;
        }
        if (html_section[pos - 1] != '<' && html_section[pos - 1] != '/') {
            continue;
        }
        if (html_section[pos - 1] == '/' && html_section[pos + name.size()] == '>') {
            k -= 1;
            if (k == 0) {
                return pos;
            }
        } else {
            if (html_section[pos - 1] == '<') {
                k += 1;
            }
        }
    }
}

string FindTagName(string html_section, int pos) {
    string res = "";
    while (html_section[pos] != '<') {
        --pos;
    }
    ++pos;
    while (html_section[pos] != ' ') {
        res += html_section[pos];
        ++pos;
    }
    return res;
}

bool isClass(string html_section, int pos) {
    while (html_section[pos] != '\"' && html_section[pos] != '\'') {
        --pos;
    }
    return html_section.substr(pos - 6, 6) == "class=";
}

vector<TagBlock> FindElementsBySubSelector(string html_section, const SubSelector &selector) {
    vector<TagBlock> elements;
    string name = selector.value;
    string valname = selector.typeval;
    if (selector.type == ' ') {
        size_t pos = 0;
        int k = 0;
        int startpos = 0;
        while (true) {
            pos = html_section.find(name, pos + 1);
            if (pos == std::string::npos) {
                break;
            }
            if (insideComment(html_section, pos)) {
                continue;
            }
            if (html_section[pos - 1] != '<' && html_section[pos - 1] != '/') {
                continue;
            }
            int j = pos;
            while (html_section[j] != ' ' && html_section[j] != '>') {
                ++j;
            }
            string str = html_section.substr(pos, j - pos);
            if (str != name) {
                continue;
            }
            if (k == 0) {
                startpos = pos;
            }
            if (html_section[pos - 1] == '/' && html_section[pos + name.size()] == '>') {
                k -= 1;
                if (k == 0) {
                    int endpos = pos;
                    string text = html_section.substr(startpos - 1, endpos - startpos + selector.value.size() + 2);
                    elements.push_back({text, startpos - 1, endpos - startpos + 3});
                }
            } else {
                if (html_section[pos - 1] == '<') {
                    k += 1;
                }
            }
        }
    }
    if ((selector.type == '.' || selector.type == '#') && valname.empty()) {
        size_t pos = 0;
        int startpos;
        while (true) {
            pos = html_section.find(name, pos + 1);
            if (pos == std::string::npos) {
                break;
            }
            if (insideComment(html_section, pos)) {
                continue;
            }
            if (html_section[pos - 1] != '\"' && html_section[pos - 1] != '\'') {
                continue;
            }
            if (selector.type == '.' && !isClass(html_section, pos)) {
                continue;
            }
            startpos = FindTagStart(html_section, pos);
            string tag_name = FindTagName(html_section, pos);
            int endpos =
                    FindTagEnd(html_section.substr(startpos, html_section.size() - startpos), tag_name) + startpos - 1;
            string text = html_section.substr(startpos, endpos - startpos + tag_name.size() + 2);
            elements.push_back({text, startpos - 1, endpos - startpos + 3});
            pos += (endpos - pos);
        }
    }
    if (!valname.empty()) {
        SubSelector sub;
        sub.type = ' ';
        sub.value = valname;
        vector<TagBlock> vec = FindElementsBySubSelector(html_section, sub);
        for (auto &i : vec) {
            sub.value = selector.typeval;
            sub.type = selector.type;
            for (const auto &j : FindElementsBySubSelector(i.text, sub)) {
                elements.push_back(j);
            }
        }
    }
    return elements;
}

bool isCombinator(char i) {
    return i == ' ' || i == '>' || i == '+' || i == ',';
}

SubSelector CreateFromString(string subSelector) {
    for (int i = 0; i < subSelector.size(); ++i) {
        if ((subSelector[i] == '.' || subSelector[i] == '#') && i != 0) {
            string v = subSelector.substr(i + 1, subSelector.size() - i); //string after dot or hashtag
            string v2 = subSelector.substr(0, i); //string before dot or hashtag
            return {subSelector[i], v2, v};
        }
        if ((subSelector[i] == '.' || subSelector[i] == '#') && i == 0) {
            string v = subSelector.substr(1, subSelector.size()); //after dot
            return {subSelector[i], v, ""};
        }
    }
    return {' ', subSelector, ""};
}

vector<TagBlock> mergeVectors(vector<TagBlock> a, const vector<TagBlock> &b) {
    for (const auto &i : b) {
        a.push_back(i);
    }
    return a;

}

vector<TagBlock> FindAllChildren(const TagBlock& block) {
    string text = block.text;
    vector<TagBlock> res;
    int pos = text.find('>');
    int k = 0;
    int endpos;
    int startpos;
    while (true) {
        pos = text.find('<', pos + 1);
        if (pos == std::string::npos) {
            break;
        }
        if (insideComment(text, pos)) {
            continue;
        }
        if (k == 0) {
            startpos = pos;
            ++k;
            continue;
        }
        if (text[pos + 1] != '/') {
            ++k;
        }
        if (text[pos + 1] == '/') {
            --k;
            if (k == 0) {
                endpos = text.find('>', pos);
                res.push_back({text.substr(startpos, endpos - startpos + 1), startpos, endpos});
            }
        }

    }
    return res;
}

bool isValid(TagBlock tag_block, SubSelector selector) {
    int tag_end = tag_block.text.find('>');
    string tag_attrs = tag_block.text.substr(1, tag_end - selector.value.size());
    int pos;
    string st = tag_block.text.substr(1, selector.value.size());
    if (selector.type == ' ') {
        if (tag_block.text.substr(1, selector.value.size()) == selector.value) {
            return true;
        }
    }
    if (selector.typeval.empty()) {
        pos = tag_attrs.find(selector.value);
        if (selector.type == '.') {
            if (pos != string::npos && isClass(tag_attrs, pos)) {
                return true;
            }
        }
        if (selector.type == '#') {
            if (pos != string::npos) {
                return true;
            }
        }
    }
    if (!selector.typeval.empty()) {
        if (tag_block.text.substr(1, selector.value.size()) == selector.value) {
            pos = tag_attrs.find(selector.typeval);
            if (selector.type == '.') {
                if (pos != string::npos && isClass(tag_attrs, pos)) {
                    return true;
                }
            }
            if (selector.type == '#') {
                if (pos != string::npos) {
                    return true;
                }
            }
        }
    }
    return false;
}

vector<TagBlock>
FindElementsByCombinator(vector<TagBlock> tag_blocks, SubSelector selector, char combinator, string html) {
    vector<TagBlock> res;
    if (combinator == ' ') {
        for (auto &tag_block : tag_blocks) {
            res = mergeVectors(res, FindElementsBySubSelector(tag_block.text, selector));
        }
    }
    if (combinator == ',') {
        res = mergeVectors(tag_blocks, FindElementsBySubSelector(html, selector));
    }
    if (combinator == '>') {
        for (auto &tag_block : tag_blocks) {
            vector<TagBlock> children = FindAllChildren(tag_block);
            for (auto child : children) {
                if (isValid(child, selector)) {
                    res.push_back(child);
                }
            }
        }
    }
    if (combinator == '+') {
        for (auto &tag_block : tag_blocks) {
            vector<TagBlock> children = FindAllChildren(tag_block);
            for(auto child : children) {

            }
        }
    }
    return res;
}

string deleteSpaces(string selector) {
    int i = 0;
    while (i < selector.size()) {
        if (isCombinator(selector[i])) {
            if (isCombinator(selector[i + 1])) {
                if (selector[i] > selector[i + 1]) {
                    selector.erase(i + 1, 1);
                    ++i;
                } else {
                    selector.erase(i, 1);
                }
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }
    return selector;
}

vector<string> ExtractHTMLAll(string html, string selector) {
    vector<string> result;
    SubSelector subSelector;
    selector = deleteSpaces(selector);
    string str_subselector = "";
    char curr_combinator;
    int flag = 0;

    vector<TagBlock> tag_blocks;
    string right_selector;
    for (char i : selector) {
        if (isCombinator(i)) {
            ++flag;
            if (flag == 1) {
                tag_blocks = FindElementsBySubSelector(html, CreateFromString(right_selector));
            } else {
                tag_blocks = FindElementsByCombinator(tag_blocks, CreateFromString(right_selector), curr_combinator,
                                                      html);
            }
            curr_combinator = i;
            right_selector = "";
        } else {
            right_selector += i;
        }
    }
    if (!flag) {
        for (const auto &i : FindElementsBySubSelector(html, CreateFromString(selector))) {
            result.push_back(i.text);
        }
    } else {
        tag_blocks = FindElementsByCombinator(tag_blocks, CreateFromString(right_selector), curr_combinator, html);
        for (const auto &i : tag_blocks) {
            result.push_back(i.text);
        }
    }
    return result;
}