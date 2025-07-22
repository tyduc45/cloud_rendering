#ifndef VERTEX_H_
#define VERTEX_H_


#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional> // For std::hash

// ���嶥��ṹ�壬����������Ҫ�Ƚϵ�����
struct Vertex {
    float* data;
    size_t float_count;
    // default constructor
    Vertex() : data(nullptr), float_count(0) {}

    Vertex(float* vertex_data, int stride) :data(vertex_data),float_count(stride) {}

    // ������������ (==)
    // ������ std::unordered_map �ܹ��ж����������Ƿ���ͬ���Ĺؼ�
    bool operator==(const Vertex& other) const {
        if (other.float_count != float_count)
        {
            return false;
        }
        return memcmp(data, other.data, float_count * sizeof(float)) == 0;
    }
};

// Ϊ Vertex �ṹ�崴��һ����ϣ�����ػ�
// ������ std::unordered_map �ܹ��� Vertex ���й�ϣ����Ĺؼ�
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            // Ϊ������vertex����hash����
            // �������hash����Ϊÿһ����������hash,Ȼ�������Ľ����hash combine
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
    const float* raw_vertices,// �������ݣ������ظ����ݣ�
    size_t num_raw_floats, // ����Ĵ�С�������������� ��λfloat��
    size_t stride,         // һ������ռ�ݶ������ݣ���λfloat��
    std::vector<float>& out_unique_vertices,
    std::vector<unsigned int>& out_indices
) {
    std::unordered_map<Vertex, unsigned int> vertex_to_index_map;

    // ���������ʵ�ʵĶ�����
    size_t num_of_vertices = num_raw_floats / stride;

    // ����ÿһ������
    for (int i = 0; i < num_of_vertices; i++)
    {
        // �������
        Vertex current_vertex(&raw_vertices[i * stride], stride);

        // ��������
        auto it = vertex_to_index_map.find(current_vertex);
        // ���û���ҵ�
        if (it == vertex_to_index_map.end())
        {
            // ˵������һ���µĶ��㣬���붥���б�,��Ҫpush_back ��current vertex��
            out_unique_vertices.insert(out_unique_vertices.end(),
                &raw_vertices[i * stride],
                &raw_vertices[i * stride] + stride);

            unsigned int index = (out_unique_vertices.size() / stride) - 1;

            out_indices.push_back(index);

            vertex_to_index_map[current_vertex] = index;
        }
        else
        {
            // �ҵ���
            out_indices.push_back(it->second);
        }
    }

}

#endif
