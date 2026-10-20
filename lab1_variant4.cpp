#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;


struct Node {
    string value;
    Node* left;
    Node* right;

    Node(string v, Node* l = nullptr, Node* r = nullptr)
        : value(v), left(l), right(r) {}

    ~Node() {
        delete left;
        delete right;
    }
};

void skipSpaces(const string& s, size_t& pos) {
    while (pos < s.size() && isspace((unsigned char)s[pos]))
        pos++;
}


Node* parseFormula(const string& s, size_t& pos) {
    skipSpaces(s, pos);
    if (pos == s.size()) return nullptr;

    if (s[pos] >= 'a' && s[pos] <= 'z') {
        string name(1, s[pos++]);
        return new Node(name);
    }

    if (s[pos] == '!') {
        pos++;
        Node* child = parseFormula(s, pos);
        if (child == nullptr) return nullptr;
        return new Node("!", child);
    }

    if (s[pos] != '(') return nullptr;
    pos++;
    Node* left = parseFormula(s, pos);
    if (left == nullptr) return nullptr;
    skipSpaces(s, pos);

    if (pos < s.size() && s[pos] == ')') {
        pos++;
        return left;
    }

    string op;
    if (s.compare(pos, 2, "->") == 0) {
        op = "->";
        pos += 2;
    } else if (pos < s.size() && (s[pos] == '&' || s[pos] == '|')) {
        op = s[pos++];
    } else {
        delete left;
        return nullptr;
    }

    Node* right = parseFormula(s, pos);
    skipSpaces(s, pos);
    if (right == nullptr || pos == s.size() || s[pos] != ')') {
        delete left;
        delete right;
        return nullptr;
    }
    pos++;
    return new Node(op, left, right);
}

Node* parse(const string& s) {
    size_t pos = 0;
    Node* root = parseFormula(s, pos);
    skipSpaces(s, pos);
    if (pos != s.size()) {
        delete root;
        return nullptr;
    }
    return root;
}

bool equalTrees(Node* a, Node* b) {
    if (a == nullptr || b == nullptr) return a == b;
    return a->value == b->value
        && equalTrees(a->left, b->left)
        && equalTrees(a->right, b->right);
}

string toString(Node* node) {
    if (node->value == "!") return "!" + toString(node->left);
    if (node->left == nullptr) return node->value;
    return "(" + toString(node->left) + " " + node->value
        + " " + toString(node->right) + ")";
}


bool matchAxiom(Node* pattern, Node* formula, map<string, Node*>& parts) {
    if (pattern == nullptr || formula == nullptr)
        return pattern == formula;

    if (pattern->left == nullptr && pattern->right == nullptr) {
        if (parts.count(pattern->value) == 0) {
            parts[pattern->value] = formula;
            return true;
        }
        return equalTrees(parts[pattern->value], formula);
    }

    return pattern->value == formula->value
        && matchAxiom(pattern->left, formula->left, parts)
        && matchAxiom(pattern->right, formula->right, parts);
}

int axiomNumber(Node* formula, const vector<Node*>& axioms) {
    for (size_t i = 0; i < axioms.size(); i++) {
        map<string, Node*> parts;
        if (matchAxiom(axioms[i], formula, parts))
            return (int)i + 1;
    }
    return 0;
}


Node* findMP(Node* formula, const vector<Node*>& known,
             const vector<Node*>& axioms) {
    for (Node* implication : known) {
        if (implication->value != "->") continue;
        if (!equalTrees(implication->right, formula)) continue;

        
        if (axiomNumber(implication->left, axioms) != 0)
            return implication;

        for (Node* previous : known) {
            if (equalTrees(implication->left, previous))
                return implication;
        }
    }
    return nullptr;
}

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    vector<string> axiomTexts = {
        "(p -> (q -> p))",
        "((s -> (p -> q)) -> ((s -> p) -> (s -> q)))",
        "((p & q) -> p)",
        "((p & q) -> q)",
        "(p -> (q -> (p & q)))",
        "(p -> (p | q))",
        "(q -> (p | q))",
        "((p -> r) -> ((q -> r) -> ((p | q) -> r)))",
        "(!p -> (p -> q))",
        "((p -> q) -> ((p -> !q) -> !p))",
        "(p | !p)"
    };

    vector<Node*> axioms;
    vector<Node*> known;
    for (const string& s : axiomTexts) {
        axioms.push_back(parse(s));
        known.push_back(parse(s));
    }

 

    string input;
    while (true) {
        cout << "\nВведите формулу: ";
        if (!getline(cin, input) || input == "exit") break;

        if (input == "list") {
            for (size_t i = 0; i < known.size(); i++)
                cout << i + 1 << ". " << toString(known[i]) << '\n';
            continue;
        }

        Node* formula = parse(input);
        if (formula == nullptr) {
            cout << "Ошибка: строка не является правильно построенной формулой.\n";
            continue;
        }

        bool repeated = false;
        for (size_t i = 0; i < known.size(); i++) {
            if (equalTrees(known[i], formula)) {
                cout << "Формула уже есть в списке под номером " << i + 1 << ".\n";
                repeated = true;
                break;
            }
        }
        if (repeated) {
            delete formula;
            continue;
        }

        int number = axiomNumber(formula, axioms);
        Node* implication = nullptr;
        if (number == 0) implication = findMP(formula, known, axioms);

        if (number != 0) {
            cout << "Формула соответствует схеме аксиомы " << number << ".\n";
        } else if (implication != nullptr) {
            cout << "Формула " << toString(formula)
                 << " выводима из формул " << toString(implication)
                 << " и " << toString(implication->left)
                 << " по правилу modus ponens.\n";
        } else {
            cout << "Формула записана правильно, но шаг не обоснован:\n";
            cout << "это не аксиома, и подходящие основания для MP не найдены.\n";
            delete formula;
            continue;
        }

        known.push_back(formula);
        cout << "Добавлена под номером " << known.size() << ".\n";
    }

    for (Node* formula : known) delete formula;
    for (Node* axiom : axioms) delete axiom;
    return 0;
}
