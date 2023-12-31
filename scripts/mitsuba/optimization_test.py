import drjit as dr
import mitsuba as mi
from matplotlib import pyplot as plt
import csv

mi.set_variant('cuda_ad_rgb')
integrator = {
    'type': 'direct_reparam',
}
from mitsuba.scalar_rgb import Transform4f as T

base_path = '../../resources/mitsuba_data/'
scene = mi.load_dict({
    'type': 'scene',
    'integrator': integrator,
    'sensor':  {
        'type': 'perspective',
        'to_world': T.look_at(
                        origin=(0, 0, 2),
                        target=(0, 0, 0),
                        up=(0, 1, 0)
                    ),
        'fov': 60,
        'film': {
            'type': 'hdrfilm',
            'width': 256,
            'height': 256,
            'rfilter': { 'type': 'gaussian' },
            'sample_border': True
        },
    },
    'wall': {
        'type': 'obj',
        'filename': base_path + 'meshes/rectangle.obj',
        'to_world': T.translate([0, 0, -2]).scale(2.0),
        'face_normals': True,
        'bsdf': {
            'type': 'diffuse',
            'reflectance': { 'type': 'rgb', 'value': (0.5, 0.5, 0.5) },
        }
    },
    'bunny': {
        'type': 'ply',
        'filename': base_path + 'meshes/bunny.ply',
        'to_world': T.scale(6.5),
        'bsdf': {
            'type': 'diffuse',
            'reflectance': { 'type': 'rgb', 'value': (0.3, 0.3, 0.75) },
        },
    },
    'light': {
        'type': 'obj',
        'filename': base_path + 'meshes/sphere.obj',
        'emitter': {
            'type': 'area',
            'radiance': {'type': 'rgb', 'value': [1e3, 1e3, 1e3]}
        },
        'to_world': T.translate([2.5, 2.5, 7.0]).scale(0.25)
    }
})

img_ref = mi.render(scene, seed=0, spp=256)

mi.util.convert_to_bitmap(img_ref)
params = mi.traverse(scene)
initial_vertex_positions = dr.unravel(mi.Point3f, params['bunny.vertex_positions'])
print(params)
print(params['bunny.vertex_positions'][0])
loss_hist = []

def F_transform(params):
    tup = tuple(params['bunny.vertex_positions'])
    params['bunny.vertex_positions'] = tup
    params.update()

def F_loss(img):
    loss = dr.sum(dr.sqr(img - img_ref)) / len(img)
    loss_hist.append(loss)
    return loss

F_transform(params)
img_init = mi.render(scene, seed=0, spp=256)
mi.util.convert_to_bitmap(img_init)
steps = 2


for it in range(steps):
  F_transform(params) # Scene = F_transform(input)
  dr.enable_grad(params['bunny.vertex_positions'])
  img = mi.render(scene, params, seed=it, spp=32) # image = F_render(scene)
  loss = F_loss(img) # loss = F_loss(image)

  dr.backward(loss) # calculate F_loss(F_render(F_transform(input))) gradient
  #print(tuple(dr.grad(params['bunny.vertex_positions'])))
  tup = tuple(params['bunny.vertex_positions'])
  with open("grad2.txt", 'w') as f:
    f.write((' '.join(['%10.6f ']*len(tup))+'\n\n') % tup)
  print(f"Iteration {it:02d}: error={loss[0]:6f}", end='\n')
  print(len(params['bunny.vertex_positions']))
  #print(f"Iteration {it:02d}: error={loss[0]:6f}, angle={opt['angle'][0]:.4f}, trans=[{opt['trans'].x[0]:.4f}, {opt['trans'].y[0]:.4f}]", end='\n')

fig, axs = plt.subplots(2, 2, figsize=(10, 10))
img_optimized = mi.render(scene, spp=256)
axs[0][0].plot(loss_hist)
axs[0][0].set_xlabel('iteration');
axs[0][0].set_ylabel('Loss');
axs[0][0].set_title('Parameter error plot');

axs[0][1].imshow(mi.util.convert_to_bitmap(dr.sqr(img - img_ref)))
axs[0][1].axis('off')
axs[0][1].set_title('Initial Image')

axs[1][0].imshow(mi.util.convert_to_bitmap(img_optimized))
axs[1][0].axis('off')
axs[1][0].set_title('Optimized image')

axs[1][1].imshow(mi.util.convert_to_bitmap(img_ref))
axs[1][1].axis('off')
axs[1][1].set_title('Reference Image')

plt.show()