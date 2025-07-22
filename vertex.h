#ifndef VERTEX_H_
#define VERTEX_H_


#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional> // For std::hash

// 定义顶点结构体，包含所有需要比较的属性
struct Vertex {
    float* data;
    size_t float_count;
    // default constructor
    Vertex() : data(nullptr), float_count(0) {}

    Vertex(float* vertex_data, int stride) :data(vertex_data),float_count(stride) {}

    // 重载相等运算符 (==)
    // 这是让 std::unordered_map 能够判断两个顶点是否“相同”的关键
    bool operator==(const Vertex& other) const {
        if (other.float_count != float_count)
        {
            return false;
        }
        return memcmp(data, other.data, float_count * sizeof(float)) == 0;
    }
};

// 为 Vertex 结构体创建一个哈希函数特化
// 这是让 std::unordered_map 能够对 Vertex 进行哈希计算的关键
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            // 为新属性vertex定义hash方法
            // 采用组合hash，先为每一个分量设置hash,然后对整体的结果做hash combine
            size_t seed = hash<size_t>{}(v.float_count);
          for(int i = 0; i < v.float_count;i++)
          {
              size_t h = hash<float>{}(v.data[i]);
              // hash combine
              seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
          }
          return seed;
        }
    };
}

void weld_vertices(
    const float* raw_vertices,// 顶点数据（包含重复数据）
    size_t num_raw_floats, // 数组的大小（包含多少数据 单位float）
    size_t stride,         // 一个顶点占据多少数据（单位float）
    std::vector<float>& out_unique_vertices,
    std::vector<unsigned int>& out_indices
) {
    std::unordered_map<Vertex, unsigned int> vertex_to_index_map;

    // 获得数据中实际的顶点数
    size_t num_of_vertices = num_raw_floats / stride;

    // 对于每一个顶点
    for (int i = 0; i < num_of_vertices; i++)
    {
        // 获得数据
        Vertex current_vertex(&raw_vertices[i * stride], stride);

        // 查找数据
        auto it = vertex_to_index_map.find(current_vertex);
        // 如果没有找到
        if (it == vertex_to_index_map.end())
        {
            // 说明这是一个新的顶点，加入顶点列表,不要push_back （current vertex）
            out_unique_vertices.insert(out_unique_vertices.end(),
                &raw_vertices[i * stride],
                &raw_vertices[i * stride] + stride);

            unsigned int index = (out_unique_vertices.size() / stride) - 1;

            out_indices.push_back(index);

            vertex_to_index_map[current_vertex] = index;
        }
        else
        {
            // 找到了
            out_indices.push_back(it->second);
        }
    }

}

#endif
