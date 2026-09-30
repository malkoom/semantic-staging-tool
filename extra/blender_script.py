import bpy
import os

# Carpeta donde quieres guardar los GLB sueltos
export_dir = "dir"
os.makedirs(export_dir, exist_ok=True)

# Deseleccionar todo
bpy.ops.object.select_all(action="DESELECT")

for obj in bpy.context.scene.objects:
    if obj.type == "MESH":
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

        # Opcional: centrar el objeto en (0,0,0) antes de exportar
        prev_loc = obj.location.copy()
        obj.location = (0, 0, 0)

        filename = f"{obj.name}.glb"
        filepath = os.path.join(export_dir, filename)

        bpy.ops.export_scene.gltf(
            filepath=filepath, use_selection=True, export_format="GLB"
        )

        # Restaurar ubicación original
        obj.location = prev_loc
        obj.select_set(False)

print("¡Exportación completada!")
