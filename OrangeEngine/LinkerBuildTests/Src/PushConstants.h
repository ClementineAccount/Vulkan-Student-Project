struct MeshPushConstants {
    glm::vec4 light_pos;
    glm::vec4 light_color;
    glm::vec4 ambient_color;
    glm::vec4 camera_pos;
    glm::mat4 model_matrix;
    glm::mat4 render_matrix;
};