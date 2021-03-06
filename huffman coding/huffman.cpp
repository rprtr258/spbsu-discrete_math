#include <string.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include <stack>
#include "node.h"
#include "freqtable.h"
#include "huffman.h"

int unsigned const alphabet = 256;
char unsigned const firstBit = (1 << 7);

int unsigned strlen(char unsigned const *str) {
    int unsigned result = 0;
    while (str[result] != '\0')
        result++;
    return result;
}

Node* getRarestNode(std::queue<Node*> &firstQueue, std::queue<Node*> &secondQueue) {
    Node *result = nullptr;
    if (firstQueue.size() == 0) {
        result = secondQueue.front();
        secondQueue.pop();
    } else if (secondQueue.size() == 0) {
        result = firstQueue.front();
        firstQueue.pop();
    } else {
        Node *tempFirst = firstQueue.front();
        Node *tempSecond = secondQueue.front();
        
        if (tempFirst->frequency < tempSecond->frequency) {
            result = tempFirst;
            firstQueue.pop();
        } else {
            result = tempSecond;
            secondQueue.pop();
        }
    }
    return result;
};

void proccessTwoRarest(std::queue<Node*> &firstQueue, std::queue<Node*> &secondQueue) {
    Node *first = getRarestNode(firstQueue, secondQueue);
    Node *second = getRarestNode(firstQueue, secondQueue);
    
    Node *parent = createNode(first, second);
    secondQueue.push(parent);
}

Node* buildTree(char const *filename) {
    std::queue<Node*> firstQueue;
    std::queue<Node*> secondQueue;
    FrequencyTable ftable(filename);
    
    for (int unsigned i = 0; i < ftable.getSize(); i++) {
        Node *leaf = createNode(ftable[i].first, ftable[i].second);
        firstQueue.push(leaf);
    }
    
    for (int unsigned i = 0; i < ftable.getSize() - 1; i++)
        proccessTwoRarest(firstQueue, secondQueue);
    
    Node *result = secondQueue.front();
    return result;
}

HuffmanTree::HuffmanTree(char const *filename) {
    root = buildTree(filename);
}

void pushNode(std::stack<Node*> &stack, char const symbol) {
    Node *node = createNode(symbol);
    stack.push(node);
}

void mergeNodes(std::stack<Node*> &stack) {
    Node *rightChild = stack.top();
    stack.pop();
    Node *leftChild = stack.top();
    stack.pop();

    Node *node = createNode(leftChild, rightChild);
    stack.push(node);
}

HuffmanTree::HuffmanTree(FILE *fileIn, int unsigned const treeSize) {
    std::stack<Node*> tempStack;
    
    char unsigned byte = 0;
    int unsigned length = 0;
    while (length < treeSize) {
        if (length % 8 == 0)
            fscanf(fileIn, "%c", &byte);
        
        int unsigned bitIndex = length % 8;
        if (byte & (1 << (7 - bitIndex))) {
            mergeNodes(tempStack);
            length++;
        } else {
            char unsigned symbol = 0;
            if (bitIndex == 7) {
                fscanf(fileIn, "%c", &symbol);
            } else {
                symbol |= (byte << (bitIndex + 1));
                fscanf(fileIn, "%c", &byte);
                symbol |= (byte >> (7 - bitIndex));
            }
            pushNode(tempStack, symbol);
            length += 9;
        }
    }
    
    root = tempStack.top();
}

HuffmanTree::~HuffmanTree() {
    if (root == nullptr)
        return;
    deleteNode(root);
}

void writeCodes(Node *node, std::vector<bool> codes[alphabet], std::vector<bool> &buffer) {
    if (isLeaf(node)) {
        codes[node->symbol] = buffer;
        return;
    }
    buffer.push_back(0);
    writeCodes(node->l, codes, buffer);
    buffer.pop_back();
    buffer.push_back(1);
    writeCodes(node->r, codes, buffer);
    buffer.pop_back();
}

void HuffmanTree::encode(char const *filename, FILE *outFile) {
    std::vector<bool> codes[alphabet];
    std::vector<bool> buffer;
    
    writeCodes(root, codes, buffer);
    
    char unsigned bitMask = firstBit;
    FILE *file = fopen(filename, "rb");
    char unsigned outByte = 0;
    bool firstWrite = true;
    while (!feof(file)) {
        char unsigned byte = 0;
        fscanf(file, "%c", &byte);
        if (feof(file))
            break;
        int unsigned const charCode = (int unsigned)byte;
        int unsigned codeLength = codes[charCode].size();
        for (int unsigned k = 0; k < codeLength; k++) {
            if (bitMask == firstBit) {
                if (firstWrite)
                    firstWrite = false;
                else
                    fprintf(outFile, "%c", outByte);
                outByte = 0;
            }
            bool bit = codes[charCode][k];
            outByte |= bit * bitMask;
            bitMask >>= 1;
            if (bitMask == 0)
                bitMask = firstBit;
        }
    }
    if (bitMask != firstBit)
        fprintf(outFile, "%c", outByte);
    fclose(file);
}

void HuffmanTree::decode(FILE *fileIn, char const *filename, int unsigned const length) {
    char unsigned bitMask = firstBit;
    int unsigned decodedBits = 0;
    char unsigned byte = 0;
    fscanf(fileIn, "%c", &byte);
    FILE *fileOut = fopen(filename, "wb");
    while (decodedBits < length) {
        char unsigned newSymbol = decodeChar(root, fileIn, byte, bitMask, decodedBits);
        fprintf(fileOut, "%c", newSymbol);
    }
    fclose(fileOut);
}

void saveNode(Node *node, ByteString &res, int unsigned &bitIndex, int unsigned &length) {
    if (node == nullptr) {
        return;
    }
    
    if (isLeaf(node)) {
        char unsigned byte = node->symbol;
        length += 9;
        if (bitIndex == 0)
            res.push_back(0);
        bitIndex = (bitIndex + 1) % 8;
        for (int unsigned i = 0; i < 8; i++) {
            if (bitIndex == 0)
                res.push_back(0);
            int unsigned bit = (byte & (1 << (7 - i)));
            res.back() |= ((bit << i) >> bitIndex);
            
            bitIndex = (bitIndex + 1) % 8;
        }
    } else {
        length++;
        saveNode(node->l, res, bitIndex, length);
        saveNode(node->r, res, bitIndex, length);
        if (bitIndex == 0)
            res.push_back(0);
        res.back() |= (1 << (7 - bitIndex));
        bitIndex = (bitIndex + 1) % 8;
    }
}

int unsigned HuffmanTree::getResultLength() {
    return calcResultLength(root);
}

ByteString HuffmanTree::asString(int unsigned &length) {
    ByteString result;
    int unsigned bitIndex = 0;
    saveNode(root, result, bitIndex, length);
    return result;
}
