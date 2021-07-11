#include "components.h"
#include "utils.h"

namespace comp {

Mesh::Mesh(std::shared_ptr<util::VertexArray>& vertexArray, unsigned int vCount, unsigned int iCount)
    : vao{vertexArray}, vertexCount{vCount}, indexCount{iCount}
{}

void Mesh::draw() {
    // if (vao.expired())
    //     return;

    const auto guard = vao->guard();
    if (vao->hasIndices())
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

}