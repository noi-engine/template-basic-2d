#version 330 core

in vec2 v_uv;

out vec4 FragColor;

uniform vec4 u_line_color;
uniform float u_cell_size;
uniform float u_line_thickness;
uniform float u_quad_size;

void main()
{
    vec2 units = v_uv * u_quad_size;
    vec2 cell_pos = mod(units, u_cell_size);

    bool on_line = cell_pos.x < u_line_thickness || cell_pos.x > (u_cell_size - u_line_thickness) ||
                   cell_pos.y < u_line_thickness || cell_pos.y > (u_cell_size - u_line_thickness);

    if (!on_line)
    {
        discard;
    }

    FragColor = u_line_color;
}
