#version 330 core

in vec2 v_uv;

out vec4 FragColor;

uniform vec4 u_color;
uniform float u_border_thickness_x;
uniform float u_border_thickness_y;

void main()
{
    vec2 dist_to_edge = min(v_uv, 1.0 - v_uv);

    if (dist_to_edge.x > u_border_thickness_x && dist_to_edge.y > u_border_thickness_y)
    {
        discard;
    }

    FragColor = u_color;
}
