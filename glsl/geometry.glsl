#version 330
#extension GL_EXT_geometry_shader4 : enable

out vec3 normal;

// given to points p1 and p2 create a vector out
// that is perpendicular to (p2-p1)
vec3 createPerp(vec3 p1, vec3 p2)
{
  vec3 invec = normalize(p2 - p1);
  vec3 ret = cross( invec, vec3(0.0, 0.0, 1.0) );
  if ( length(ret) == 0.0 )
  {
    ret = cross( invec, vec3(0.0, 1.0, 0.0) );
  }
  return ret;
}

void main()
{
   mat4 mvp = gl_ModelViewProjectionMatrix;


   float r1 = gl_PositionIn[0].w;
   float r2 = gl_PositionIn[1].w;

   vec3 axis = gl_PositionIn[1].xyz - gl_PositionIn[0].xyz;

   vec3 perpx = createPerp( gl_PositionIn[1].xyz, gl_PositionIn[0].xyz );
   vec3 perpy = cross( normalize(axis), perpx );
   int segs = 16;
   for(int i=0; i<segs; i++) {
      float a = i/float(segs-1) * 2.0 * 3.14159;
      float ca = cos(a); float sa = sin(a);
      normal = vec3( ca*perpx.x + sa*perpy.x,
                     ca*perpx.y + sa*perpy.y,
                     ca*perpx.z + sa*perpy.z );

      vec3 p1 = gl_PositionIn[0].xyz + r1*normal;
      vec3 p2 = gl_PositionIn[1].xyz + r2*normal;

      gl_Position = mvp * vec4(p1, 1.0); EmitVertex();
      gl_Position = mvp * vec4(p2, 1.0); EmitVertex();
   }
   EndPrimitive();
}
