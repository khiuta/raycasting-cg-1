#include "../../utils/ListMesh.hpp"
#include "../../utils/BVH.hpp"
#include <iostream>
#include "../../utils/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
ListMesh::ListMesh() {};

ListMesh::ListMesh(const std::string &filename) {
  texture->filename = filename;
  texture->loadTexture();
};

ListMesh::ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb)
  : faces(std::move(faces)), vertices(std::move(vertices)), centroid(centroid), aabb(std::move(aabb)) {

  };

bool ListMesh::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  return aabb.Hit(origin, dir, t_min, t_max, hr);
}

void ListMesh::rebuildStructures() {
    if (faces.empty()) return;

    // repopulate the aabb
    this->aabb.t.clear();
    for(auto& face : faces) {
        this->aabb.t.push_back(face.get());
    }

    float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
    float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;

    for (auto& t : faces) {
        float xs[] = {t->p1->x, t->p2->x, t->p3->x};
        float ys[] = {t->p1->y, t->p2->y, t->p3->y};
        float zs[] = {t->p1->z, t->p2->z, t->p3->z};

        for(int i = 0; i < 3; i++) {
            if(xs[i] < minX) minX = xs[i];
            if(xs[i] > maxX) maxX = xs[i];
            if(ys[i] < minY) minY = ys[i];
            if(ys[i] > maxY) maxY = ys[i];
            if(zs[i] < minZ) minZ = zs[i];
            if(zs[i] > maxZ) maxZ = zs[i];
        }
    }

    this->aabb.min_x = minX; this->aabb.max_x = maxX;
    this->aabb.min_y = minY; this->aabb.max_y = maxY;
    this->aabb.min_z = minZ; this->aabb.max_z = maxZ;

    this->centroid = Point4((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f, 1.0f);

    this->aabb.buildBVH(10); 
}

void ListMesh::applyTranslate(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

void ListMesh::applyScale(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

void ListMesh::applyRotation(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

void ListMesh::applyShear(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

std::vector<float> ListMesh::FlattenVertices() {
    std::vector<float> flatData;
    flatData.reserve(vertices.size() * 3); 

    for (const auto& pt : vertices) {
        flatData.push_back(pt->x);
        flatData.push_back(pt->y);
        flatData.push_back(pt->z);
    }
    
    return flatData;
}

void ListMesh::InitBuffers() {
    if (vertices.empty() || indices.empty()) return;

    auto flatData = FlattenVertices();

    // making data type compatibility for the EBO
    std::vector<unsigned int> glIndices(indices.begin(), indices.end());

    // create VAO
    glGenVertexArrays(1, &this->VAO);
    glBindVertexArray(this->VAO);

    // send flatData vertices
    glGenBuffers(1, &this->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, flatData.size() * sizeof(float), flatData.data(), GL_STATIC_DRAW);

    // send link order
    glGenBuffers(1, &this->EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, glIndices.size() * sizeof(unsigned int), glIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    this->shader = std::make_unique<ShaderRC>("../shaders/default.vs", "../shaders/default.fs");
    
    //unbind VAO
    glBindVertexArray(0); 

    std::cout << "EBO Iniciado! Vértices: " << vertices.size() << " | Índices: " << indices.size() << std::endl;
}
void ListMesh::UpdateBuffers() {
    if(this->VBO == 0) return;
    auto flatData = FlattenVertices();
    
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, flatData.size() * sizeof(float), flatData.data(), GL_STATIC_DRAW);
}
void ListMesh::Draw(glm::mat4 view, float fov) {
    if (this->VAO == 0 || this->indices.empty()) return;
    glEnable(GL_DEPTH_TEST);
    
    this->shader->use();
   
    
    glm::mat4 model = glm::mat4(1.0f);
    
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)800 / (float)600, 0.1f, 1000.0f);
    glm::mat4 mvp = projection * view * model;

    this->shader->setMat4("u_MVP",mvp);

   
    glBindVertexArray(this->VAO);
    
    
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, (void*)0);
    
    glBindVertexArray(0);
}