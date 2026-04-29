//
// Created by Surtr Muelsyse on 2026/4/29.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <deque>
#include <list>

using namespace std;

// 常量定义
const int TOTAL_INSTRUCTIONS = 400; // 总指令数 [cite: 60]
const int MIN_FRAMES = 4;           // 最小页框数 [cite: 64]
const int MAX_FRAMES = 40;          // 最大页框数 [cite: 64]

// 函数声明
double simulateFIFO(const vector<int>& pages, int frameSize);
double simulateLRU(const vector<int>& pages, int frameSize);
double simulateOPT(const vector<int>& pages, int frameSize);

int main() {
    vector<int> instructions(TOTAL_INSTRUCTIONS);
    vector<int> pages(TOTAL_INSTRUCTIONS);

    // 1. 读取测试文件 test.txt [cite: 90]
    ifstream file("test.txt");
    if (!file.is_open()) {
        cerr << "错误：无法打开 test.txt 文件！" << endl;
        return 1;
    }

    for (int i = 0; i < TOTAL_INSTRUCTIONS; ++i) {
        if (!(file >> instructions[i])) break;
        // 2. 地址转换：页号 = 指令地址 / 10 取整
        pages[i] = instructions[i] / 10;
    }
    file.close();

    cout << "Frame Count    OPT Hit Rate    FIFO Hit Rate    LRU Hit Rate" << endl;
    // 3. 循环遍历页框数从 4 到 40 [cite: 66]
    for (int f = MIN_FRAMES; f <= MAX_FRAMES; ++f) {
        cout << "[" << f << "]  ";

        // 计算各算法命中率并按格式输出 [cite: 69, 75-77]
        cout << fixed << setprecision(4);
        cout << "OPT: " << simulateOPT(pages, f) << "  FIFO: " << simulateFIFO(pages, f) << "  LRU: " << simulateLRU(pages, f) << endl;
    }

    return 0;
}

/**
 * FIFO (先进先出) 算法实现
 */
double simulateFIFO(const vector<int>& pages, int frameSize) {
    deque<int> memory;
    int misses = 0;

    for (int page : pages) {
        bool hit = false;
        for (int m : memory) {
            if (m == page) {
                hit = true;
                break;
            }
        }

        if (!hit) {
            misses++; // 只要不在内存中，无论是否满都计为缺页
            if (memory.size() >= frameSize) {
                memory.pop_front(); // 移除最早进入的页面
            }
            memory.push_back(page);
        }
    }
    return 1.0 - (double)misses / TOTAL_INSTRUCTIONS; // 命中率 = 1 - 缺页率 [cite: 67]
}

/**
 * LRU (最近最久未使用) 算法实现
 */
double simulateLRU(const vector<int>& pages, int frameSize) {
    list<int> memory; // 链表尾部表示最近访问
    int misses = 0;

    for (int page : pages) {
        auto it = find(memory.begin(), memory.end(), page);

        if (it != memory.end()) {
            // 命中：移动到链表末尾
            memory.erase(it);
            memory.push_back(page);
        } else {
            // 缺页
            misses++;
            if (memory.size() >= frameSize) {
                memory.pop_front(); // 移除最久未使用的（头部）
            }
            memory.push_back(page);
        }
    }
    return 1.0 - (double)misses / TOTAL_INSTRUCTIONS;
}

/**
 * OPT (最佳置换) 算法实现
 */
double simulateOPT(const vector<int>& pages, int frameSize) {
    vector<int> memory;
    int misses = 0;

    for (int i = 0; i < TOTAL_INSTRUCTIONS; ++i) {
        int currentPage = pages[i];
        auto it = find(memory.begin(), memory.end(), currentPage);

        if (it == memory.end()) {
            misses++; // 缺页
            if (memory.size() < frameSize) {
                memory.push_back(currentPage);
            } else {
                // 寻找未来最长时间不使用的页面
                int victimIdx = -1;
                int furthestAccess = -1;

                for (int m = 0; m < memory.size(); ++m) {
                    int nextUse = TOTAL_INSTRUCTIONS + 1;
                    for (int j = i + 1; j < TOTAL_INSTRUCTIONS; ++j) {
                        if (pages[j] == memory[m]) {
                            nextUse = j;
                            break;
                        }
                    }
                    if (nextUse > furthestAccess) {
                        furthestAccess = nextUse;
                        victimIdx = m;
                    }
                }
                memory[victimIdx] = currentPage;
            }
        }
    }
    return 1.0 - (double)misses / TOTAL_INSTRUCTIONS;
}