"""
Blender .buvf file format export script.
"""


import bpy
import bmesh


def write_bcf_data(filepath):
    # get selected object mesh
    print("Starting to write data-----------------")
    me = bpy.context.active_object.data
    bm = bmesh.new()
    bm.from_mesh(me)

    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    # face count after triangulation
    triCount = len(bm.faces)
    bm.to_mesh(me)
    bm.free()
    
    me.calc_loop_triangles()
    str_vertices = []
    str_normals = []
    str_uvs = []
    print(f"Found {triCount} triangles")
    print(f"num triangles from loop_triangles: {len(me.loop_triangles)}")
    for tri in me.loop_triangles:
        for vert_index, loop_index in zip(tri.vertices, tri.loops):
            l = me.uv_layers.active.data[loop_index].uv
            v = me.vertices[vert_index]
            str_vertices.append(f"{v.co[0]} {v.co[1]} {v.co[2]}")
            str_normals.append(f"{v.normal[0]} {v.normal[1]} {v.normal[2]}")
            str_uvs.append(f"{l[0]} {l[1]}")
    #str_vertices = [f"{v.co[0]} {v.co[1]} {v.co[2]}" for v in me.vertices]
    #str_normals = [f"{v.normal[0]} {v.normal[1]} {v.normal[2]}" for v in me.vertices]
    print(f"Num vertices: {len(str_vertices)}")
    f = open(filepath, "w", encoding='utf-8')
    f.write(f"{len(str_vertices)} ")
    f.write(f"{' '.join(str_vertices)} {' '.join(str_normals)} {' '.join(str_uvs)}")
    f.close()

    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportBCFData(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_test.some_data"  # Important since its how bpy.ops.import_test.some_data is constructed.
    bl_label = "Export mesh in BCF format"

    # ExportHelper mix-in class uses this.
    filename_ext = ".buvf"

    filter_glob: StringProperty(
        default="*.bcf",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return write_bcf_data(self.filepath)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportBCFData.bl_idname, text="BCF Export Operator")


# Register and add to the "file selector" menu (required to use F3 search "Text Export Operator" for quick access).
def register():
    bpy.utils.register_class(ExportBCFData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportBCFData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # Test call.
    bpy.ops.export_test.some_data('INVOKE_DEFAULT')
