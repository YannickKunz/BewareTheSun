"""Author the game's original meshes and animation in Blender, then export to Godot.

blender --background --factory-startup --python tools/build_models.py
Add -- --preview to render the character showcase as well.
No imported source meshes, stock assets, or Godot geometry are used.
"""
from pathlib import Path
import bpy
import math
import random
import json
import hashlib
import sys
import shutil
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'game/assets/models'
SOURCE = ROOT / 'art-source'
OUT.mkdir(parents=True, exist_ok=True)
SOURCE.mkdir(exist_ok=True)
random.seed(731)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for block in list(bpy.data.materials):
    bpy.data.materials.remove(block)
bpy.context.scene.name = 'Garden asset library'
bpy.context.scene.render.fps = 30
bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 61


def linear(v):
    return v / 12.92 if v < .04045 else ((v + .055) / 1.055) ** 2.4


def mat(name, hexcode, rough=.7, metal=0, glow=0):
    color = tuple(linear(int(hexcode[i:i+2],16)/255) for i in (0,2,4))
    m=bpy.data.materials.new(name); m.use_nodes=True
    m.diffuse_color=(*color,1)
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Roughness'].default_value=rough
    p.inputs['Metallic'].default_value=metal
    if glow:
        p.inputs['Emission Color'].default_value=(*color,1)
        p.inputs['Emission Strength'].default_value=glow
    return m

CLAY=mat('Terracotta | hand-thrown','B96344')
RIM=mat('Terracotta | worn edges','DB9666')
DARK_CLAY=mat('Terracotta | incised bands','874A39')
EARTH=mat('Rich potting soil','40372E')
IVORY=mat('Jasmine | warm ivory','F6EDCD',.58)
PETAL_SHADOW=mat('Jasmine | shaded petal','D4D5AC')
GOLD=mat('Pollen | saffron','E8B753',.55)
LEAF=mat('Foliage | jade','38674B')
LEAF_LIGHT=mat('Foliage | fresh tips','779353')
VEIN=mat('Foliage | leaf veins','A1B373')
WOOD=mat('Cedar | honey bark','816040')
ROOT_MAT=mat('Roots | umber','654C35')
EYES=mat('Eyes | deep espresso','142B29',.24)
BLUSH=mat('Cheeks | rose clay','E39873')
BRASS=mat('Lantern | antique brass','A88042',.38,.58)
GLASS=mat('Lantern | moonstone','8BDED8',.2,.1,.35)
BEETLE=mat('Beetle | blue-black shell','263F4D',.4,.18)
WING=mat('Beetle | petrol carapace','4A7180',.35,.2)
GROOVE=mat('Beetle | dark seams','172B36')
BUG_EYE=mat('Beetle | amber eyes','F1B25E',.23,0,.35)
STONE=mat('Stone | warm limestone','B5A785')
CAP=mat('Stone | chipped edges','D0C2A0')
MORTAR=mat('Stone | crevices','807860')
MOSS=mat('Moss | soft lichen','6C8260')
SOIL=mat('Ground | sandy loam','96896A')
STRATA=mat('Ground | earth strata','665C48')
WATER=mat('Water | jade blue','416E78',.25,.15)
DEW=mat('Dew | turquoise crystal','75D5D5',.17,.15,.18)
MOON=mat('Moonflower | glacial blue','A6D7DE',.48,0,.09)
SHROOM=mat('Mushroom | muted persimmon','B46D4E')
SHADE=mat('Shelter | moss inlay','5E7860')

assets={}
collection=None


def group(name, parent=None, loc=(0,0,0)):
    n=bpy.data.objects.new(name,None); collection.objects.link(n)
    n.parent=parent; n.location=loc; n.empty_display_size=.15
    return n


def begin(name):
    global collection
    collection=bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(collection)
    root=group(name)
    assets[name]={'root':root,'collection':collection,'clips':[]}
    return root


def finish_object(obj,name,parent,material=None):
    obj.name=name
    for coll in list(obj.users_collection): coll.objects.unlink(obj)
    collection.objects.link(obj)
    obj.parent=parent
    if material: obj.data.materials.append(material)
    return obj


def sphere(name,parent,loc,scale,material,segments=16,rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,location=loc)
    o=finish_object(bpy.context.object,name,parent,material);o.scale=scale
    for p in o.data.polygons:p.use_smooth=True
    return o


def ico(name,parent,loc,scale,material,subdiv=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv,radius=1,location=loc)
    o=finish_object(bpy.context.object,name,parent,material);o.scale=scale
    return o


def cube(name,parent,loc,size,material,bevel=.04):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    o=finish_object(bpy.context.object,name,parent,material);o.scale=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Soft chipped edges','BEVEL');mod.width=bevel;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod=o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL')
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return o


def cylinder(name,parent,loc,radius,depth,material,top=None,vertices=20):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices,radius1=radius,radius2=radius if top is None else top,depth=depth,location=loc)
    return finish_object(bpy.context.object,name,parent,material)


def torus(name,parent,loc,radius,thickness,material,rotation=(0,0,0)):
    bpy.ops.mesh.primitive_torus_add(major_segments=24,minor_segments=6,location=loc,major_radius=radius,minor_radius=thickness)
    o=finish_object(bpy.context.object,name,parent,material);o.rotation_euler=rotation
    for p in o.data.polygons:p.use_smooth=True
    return o


def curve(name,parent,points,radius,material):
    data=bpy.data.curves.new(name,'CURVE');data.dimensions='3D';data.resolution_u=8
    data.bevel_depth=radius;data.bevel_resolution=1
    spline=data.splines.new('BEZIER');spline.bezier_points.add(len(points)-1)
    for bp,coord in zip(spline.bezier_points,points):
        bp.co=coord;bp.handle_left_type='AUTO';bp.handle_right_type='AUTO'
    o=bpy.data.objects.new(name,data);collection.objects.link(o);o.parent=parent;data.materials.append(material)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.object.convert(target='MESH')
    return bpy.context.object


def lathe(name,parent,profile,material,segments=32):
    verts=[];faces=[]
    for r,z in profile:
        verts.extend((r*math.cos(i*math.tau/segments),r*math.sin(i*math.tau/segments),z) for i in range(segments))
    for j in range(len(profile)-1):
        for i in range(segments):
            a=j*segments+i;b=j*segments+(i+1)%segments
            faces.append((a,b,b+segments,a+segments))
    data=bpy.data.meshes.new(name);data.from_pydata(verts,[],faces);data.update()
    o=bpy.data.objects.new(name,data);collection.objects.link(o);o.parent=parent;data.materials.append(material)
    for p in data.polygons:p.use_smooth=True
    return o


def leaf(name,parent,base,tip,width,material=LEAF,vein=True):
    # A folded lancet rather than an ellipsoid: gently cupped center seam.
    a,b=Vector(base),Vector(tip);d=b-a
    side=d.cross(Vector((0,-1,.4))).normalized()*width
    verts=[a,a+d*.35+side,a+d*.7+side*.63,b,a+d*.7-side*.63,a+d*.35-side,a+d*.46+Vector((0,-.045,.045))]
    faces=[(0,1,6),(1,2,6),(2,3,6),(3,4,6),(4,5,6),(5,0,6)]
    data=bpy.data.meshes.new(name);data.from_pydata(verts,[],faces);data.update()
    o=bpy.data.objects.new(name,data);collection.objects.link(o);o.parent=parent;data.materials.append(material)
    # Double-sided geometry works consistently in glTF and the compatibility renderer.
    mod=o.modifiers.new('Leaf thickness','SOLIDIFY');mod.thickness=.012
    bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
    if vein:curve(name+' midrib',parent,[a,a+d*.46+Vector((0,-.05,.047)),b],.009,VEIN)
    return o


def jasmine(parent,center,size=1,tilt=.38):
    bloom=group('Jasmine blossom',parent,center);bloom.rotation_euler.x=tilt
    for i in range(5):
        a=i*math.tau/5
        pet=sphere('Sculpted petal',bloom,(math.cos(a)*.18*size,math.sin(a)*.18*size,.025),(.23*size,.105*size,.05*size),IVORY)
        pet.rotation_euler=(0,-.13,a+.27)
    sphere('Pollen center',bloom,(0,0,.075*size),(.071*size,)*3,GOLD,12,6)
    return bloom


def join_meshes(parent,name):
    meshes=[o for o in parent.children if o.type=='MESH']
    if not meshes:return
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:o.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0]
    bpy.ops.object.join();meshes[0].name=name


def animate(asset_name,clip,objects,duration,pose):
    # Each object's matching NLA track becomes part of one glTF animation clip.
    frames=int(duration*30)
    base={o:(o.location.copy(),o.rotation_euler.copy(),o.scale.copy()) for o in objects}
    for o in objects:
        o.animation_data_create();o.animation_data.action=None
    for frame in range(1,frames+2):
        t=(frame-1)/frames
        for o,(loc,rot,scale) in base.items():
            o.location=loc;o.rotation_euler=rot;o.scale=scale
        pose(t)
        for o in objects:
            o.keyframe_insert(data_path='location',frame=frame)
            o.keyframe_insert(data_path='rotation_euler',frame=frame)
            o.keyframe_insert(data_path='scale',frame=frame)
    for o in objects:
        action=o.animation_data.action;action.name=f'{asset_name} | {clip} | {o.name}'
        slot=o.animation_data.action_slot
        track=o.animation_data.nla_tracks.new();track.name=clip
        strip=track.strips.new(clip,1,action);strip.action_slot=slot
        o.animation_data.action=None;track.mute=True
        o.location,o.rotation_euler,o.scale=base[o]
    assets[asset_name]['clips'].append(clip)


# PETIT JASMIN — hand-thrown pot, root boots, folded foliage, pinwheel flowers.
r=begin('petit_jasmin');body=group('JasminBody',r)
lathe('Wheel-thrown pot',body,[(.24,.14),(.29,.16),(.33,.25),(.38,.55),(.425,.76),(.432,.81),(.385,.81),(.355,.70)],CLAY)
torus('Rolled terracotta lip',body,(0,0,.79),.414,.061,RIM)
torus('Lower incised band',body,(0,0,.27),.323,.012,DARK_CLAY)
torus('Upper incised band',body,(0,0,.65),.406,.011,DARK_CLAY)
cylinder('Soil',body,(0,0,.756),.37,.035,EARTH)
for i in range(13):
    a=random.random()*math.tau;d=random.random()*.29
    ico('Soil pebble',body,(math.cos(a)*d,math.sin(a)*d,.782),(.026,.018,.013),ROOT_MAT)
for side in [-1,1]:
    sphere('Bright eye',body,(side*.145,-.378,.51),(.067,.034,.086),EYES)
    sphere('Eye reflection',body,(side*.145-.018,-.41,.538),(.021,.009,.025),IVORY,12,6)
    sphere('Clay cheek',body,(side*.265,-.303,.40),(.063,.019,.031),BLUSH,12,6)
curve('Smile',body,[(-.062,-.405,.38),(0,-.417,.356),(.067,-.402,.38)],.016,DARK_CLAY)
# A tiny stamped leaf on the back of the pot.
leaf('Pot makers leaf seal',body,(-.10,.375,.44),(.12,.405,.61),.05,RIM,False)
join_meshes(body,'HandmadePot')
feet=[]
for side,name in [(-1,'JasminFootL'),(1,'JasminFootR')]:
    foot=group(name,r,(side*.18,0,.105));feet.append(foot)
    sphere('Root boot',foot,(0,-.065,0),(.125,.23,.095),ROOT_MAT)
    for i in range(3):curve('Root toe',foot,[((i-1)*.055,-.12,.01),((i-1)*.063,-.25,-.01),((i-1)*.055,-.275,.005)],.024,WOOD)
    join_meshes(foot,name+'Mesh')
stem=group('JasminStem',body,(0,0,.78))
curve('Curving jasmine stem',stem,[(0,0,0),(-.02,0,.30),(.07,.015,.62),(.025,0,.94)],.027,LEAF)
for side,z,length in [(-1,.19,.39),(1,.36,.43),(-1,.53,.33)]:
    leaf('Folded jasmine leaf',stem,(.02,0,z),(side*length,-.025,z+.13),.12,LEAF_LIGHT if side==1 else LEAF)
curve('Second flower stem',stem,[(0,0,.30),(-.22,0,.56),(-.28,-.01,.64)],.019,LEAF)
jasmine(stem,(.025,0,.98),1.05,.6)
jasmine(stem,(-.29,-.01,.65),.70,.45)
jasmine(stem,(.28,.05,.61),.49,.55)
join_meshes(stem,'JasmineFoliage')
arm=group('LanternArm',body)
curve('Leaf arm',arm,[(.35,.02,.60),(.54,-.07,.55),(.60,-.09,.56)],.032,LEAF)
leaf('Arm leaf',arm,(.44,0,.55),(.69,.01,.72),.06,LEAF_LIGHT)
lantern=group('JasminLantern',arm,(.65,-.08,.43))
cylinder('Lantern foot',lantern,(0,0,0),.14,.055,BRASS)
cylinder('Lantern cap',lantern,(0,0,.28),.14,.075,BRASS,.07)
for i in range(4):
    a=i*math.tau/4+math.pi/4
    curve('Brass cage',lantern,[(math.cos(a)*.10,math.sin(a)*.10,.025),(math.cos(a)*.105,math.sin(a)*.105,.24)],.015,BRASS)
torus('Lantern handle',lantern,(0,0,.37),.084,.014,BRASS,(math.pi/2,0,0))
join_meshes(lantern,'LanternBrass')
sphere('LanternLens',lantern,(0,0,.145),(.084,.084,.112),GLASS)
animate('petit_jasmin','idle',[body,stem,lantern,*feet],2.4,lambda t:(setattr(body.location,'z',math.sin(t*math.tau)*.018),setattr(stem.rotation_euler,'y',math.sin(t*math.tau)*.055),setattr(lantern.rotation_euler,'y',math.sin(t*math.tau+.5)*.045)))
def walk_pose(t):
    a=t*math.tau;body.location.z=abs(math.sin(a))*.052;body.rotation_euler.y=math.sin(a)*.045
    stem.rotation_euler.y=math.sin(a+.7)*.10;lantern.rotation_euler.y=math.sin(a)*.12
    for idx,foot in enumerate(feet):
        b=a+idx*math.pi;foot.location.y=-math.sin(b)*.12;foot.location.z=.105+max(0,math.cos(b))*.075;foot.rotation_euler.x=math.sin(b)*.23
animate('petit_jasmin','walk',[body,stem,lantern,*feet],.72,walk_pose)

# DUSK BEETLE — split carapace, inset seams, articulated tripod gait.
r=begin('dusk_beetle');shell=group('BeetleBody',r)
sphere('Beetle abdomen',shell,(0,.06,.33),(.39,.52,.26),BEETLE)
for side in [-1,1]:
    sphere('Split wing',shell,(side*.17,.09,.47),(.225,.46,.17),WING)
    curve('Wing engraving',shell,[(side*.27,-.13,.58),(side*.29,.12,.61),(side*.18,.43,.56)],.011,BRASS)
sphere('Head armor',shell,(0,-.43,.32),(.29,.23,.24),BEETLE)
for side in [-1,1]:
    sphere('Amber beetle eye',shell,(side*.13,-.619,.40),(.065,.043,.057),BUG_EYE)
    sphere('Eye glint',shell,(side*.13-.014,-.654,.418),(.014,.006,.014),IVORY,10,6)
    curve('Feelers',shell,[(side*.14,-.48,.46),(side*.22,-.56,.70),(side*.30,-.66,.76)],.017,ROOT_MAT)
    sphere('Antenna tip',shell,(side*.30,-.66,.76),(.03,)*3,BUG_EYE,10,6)
join_meshes(shell,'Carapace')
legs=[]
for side in [-1,1]:
    for i in range(3):
        leg=group(f'BeetleLeg{side}_{i}',r,(side*.29,(i-1)*.28,.30));legs.append(leg)
        curve('Upper leg',leg,[(0,0,0),(side*.24,.07,.01)],.035,BEETLE)
        curve('Lower leg',leg,[(side*.24,.07,.01),(side*.32,.14,-.25)],.026,ROOT_MAT)
        sphere('Knee',leg,(side*.24,.07,.01),(.05,)*3,WING,10,6)
        join_meshes(leg,leg.name+'Mesh')
def beetle_walk(t):
    shell.location.z=abs(math.sin(t*math.tau))*.026
    for i,o in enumerate(legs):
        a=t*math.tau+(i%2)*math.pi+(math.pi if i>=3 else 0)
        o.rotation_euler.z=math.sin(a)*.22;o.rotation_euler.y=math.cos(a)*.10
animate('dusk_beetle','scuttle',[shell,*legs],.8,beetle_walk)
def beetle_sleep(t):
    shell.location.z=-.09+math.sin(t*math.tau)*.009
    for i,o in enumerate(legs):o.rotation_euler.y=.25 if i<3 else -.25
animate('dusk_beetle','sleep',[shell,*legs],3,beetle_sleep)

# UMBRELLA TREE — twisting trunk, exposed roots, broad layered leaf canopy.
r=begin('umbrella_tree')
curve('Twisted cedar trunk',r,[(0,0,0),(.10,.02,.6),(-.07,.03,1.4),(.03,0,2.3)],.13,WOOD)
for i in range(7):
    a=i*math.tau/7
    curve('Exposed root',r,[(.01,0,.25),(math.cos(a)*.32,math.sin(a)*.32,.08),(math.cos(a)*.61,math.sin(a)*.61,.025)],.048,WOOD)
    curve('Open branch',r,[(0,0,1.5),(math.cos(a)*.49,math.sin(a)*.49,2.07),(math.cos(a)*.99,math.sin(a)*.99,2.40)],.062,WOOD)
join_meshes(r,'TreeTrunk')
crown=group('TreeCanopy',r,(0,0,2.5))
for i in range(22):
    a=i*2.4;d=.38+(.85 if i<15 else .1)*((i%5+1)/5)
    c=(math.cos(a)*d,math.sin(a)*d,.08+(.28 if i>14 else 0))
    ico('Canopy leaf cushion',crown,c,(.70,.65,.35),[LEAF,LEAF_LIGHT,MOSS][i%3],2)
for i in range(26):
    a=random.random()*math.tau;d=random.uniform(.4,1.4)
    loc=Vector((math.cos(a)*d,math.sin(a)*d,.28))
    leaf('Canopy tips',crown,loc,loc+Vector((math.cos(a)*.28,math.sin(a)*.28,.04)),.11,LEAF_LIGHT,False)
join_meshes(crown,'CanopyCrown')
animate('umbrella_tree','sway',[crown],4,lambda t:(setattr(crown.rotation_euler,'x',math.sin(t*math.tau)*.024),setattr(crown.rotation_euler,'y',math.cos(t*math.tau)*.018)))

# LIMESTONE HOME ARCH with a real open semicircular arch and vines.
r=begin('garden_gate')
for side in [-1,1]:
    cube('Pillar plinth',r,(side*.85,0,.13),(.58,.60,.26),STONE,.055)
    for i in range(4):cube('Pillar block',r,(side*.85,0,.44+i*.38),(.36,.43,.35),STONE,.035)
    cube('Pillar capital',r,(side*.85,0,1.86),(.53,.55,.18),CAP,.04)
for i in range(13):
    a=(i+.5)*math.pi/13
    o=cube('Arch voussoir',r,(math.cos(a)*.86,0,1.84+math.sin(a)*.86),(.25,.43,.29),CAP if i%3 else STONE,.027)
    o.rotation_euler.y=math.pi/2-a
curve('Climbing vine',r,[(-.9,-.26,.17),(-.72,-.26,.75),(-.99,-.22,1.37),(-.72,-.24,2.22),(0,-.23,2.77),(.72,-.24,2.22),(.86,-.24,1.70)],.033,LEAF)
for i in range(13):
    a=i*math.pi/12
    p=Vector((math.cos(a)*.94,-.27,1.83+math.sin(a)*.96))
    leaf('Arch ivy',r,p,p+Vector(((-1)**i*.23,-.02,.20)),.095,LEAF_LIGHT if i%3 else LEAF,False)
for p in [(-.8,-.29,2.05),(.62,-.29,2.48)]:jasmine(r,p,.5,.85)
join_meshes(r,'LimestoneArch')

# TWO POLARITY FLOWERS. Head is an explicit pivot driven by gameplay charge.
for name,petal_mat,heart_mat in [('sunflower',GOLD,ROOT_MAT),('moonflower',MOON,WATER)]:
    r=begin(name)
    curve('Flower stem',r,[(0,0,0),(.04,.01,.5),(0,0,1.12)],.035,LEAF)
    leaf('Left bract',r,(0,0,.39),(-.35,-.025,.63),.105,LEAF)
    leaf('Right bract',r,(.02,0,.61),(.35,-.025,.85),.10,LEAF_LIGHT)
    join_meshes(r,'FlowerStem')
    head=group('SunflowerHead' if name=='sunflower' else 'MoonflowerHead',r,(0,0,1.14))
    for i in range(12):
        a=i*math.tau/12
        p=sphere('Outer petal',head,(math.cos(a)*.30,-.01,math.sin(a)*.30),(.225,.063,.079),petal_mat,12,6)
        p.rotation_euler.y=-a
    sphere('Seed disc',head,(0,-.055,0),(.22,.10,.22),heart_mat,20,10)
    for i in range(19):
        a=i*2.4;d=.15*math.sqrt(i/19)
        sphere('Seed',head,(math.cos(a)*d,-.15,math.sin(a)*d),(.022,.015,.022),GOLD if name=='sunflower' else IVORY,8,4)
    join_meshes(head,'PetalCrown')

# TERRAIN — an inset miniature earthen slab with a sculpted chamfered edge.
r=begin('garden_base')
cube('Earthen foundation',r,(0,0,-.56),(23.35,15.35,1.05),STRATA,.22)
cube('Soil horizon',r,(0,0,-.22),(23.65,15.65,.28),SOIL,.14)
cube('GroundSurface',r,(0,0,-.045),(23.0,15.0,.09),SOIL,.055)
for i in range(35):
    x=random.uniform(-11.1,11.1)
    ico('Embedded river stone',r,(x,-7.65,random.uniform(-.85,-.40)),(.24,.085,.10),MORTAR,1)
# Do not join GroundSurface: Godot applies its dynamic lantern shader only here.

r=begin('path_tile')
cube('Worn stepping stone',r,(0,0,.025),(.68,.60,.07),CAP,.065)
curve('Fine stone crack',r,[(-.19,.28,.064),(-.11,.16,.064),(-.16,.09,.064)],.005,MORTAR)
join_meshes(r,'PathTile')

r=begin('border_stone')
cube('Beveled garden edging',r,(0,0,.085),(.94,.29,.19),STONE,.035)
join_meshes(r,'BorderStone')

r=begin('wall_segment')
cube('Mortar core',r,(0,0,.31),(.98,.75,.61),MORTAR,.02)
for row in range(2):
    for i in range(2):
        cube('Hand cut limestone',r,((i-.5)*.48,0,.16+row*.30),(.46,.79,.28),STONE if i!=row else CAP,.035)
cube('Coping stone',r,(0,0,.69),(1.07,.87,.15),CAP,.04)
for side in [-1,1]:
    ico('Lichen patch',r,(.27,side*.40,.12),(.23,.025,.11),MOSS,1)
join_meshes(r,'GardenWall')

r=begin('pond')
cube('Pool stone rim',r,(0,0,.035),(1.08,1.08,.10),STONE,.07)
cube('StillWater',r,(0,0,.087),(1,1,.018),WATER,.045)
join_meshes(r,'Pool')

r=begin('lily_pad')
cylinder('Lily pad',r,(0,0,.02),.25,.025,LEAF_LIGHT,vertices=16)
curve('Pad midrib',r,[(0,0,.038),(.2,0,.038)],.008,VEIN)
jasmine(r,(-.07,0,.11),.40,0)
join_meshes(r,'WaterLily')

r=begin('grass_clump')
for i in range(9):
    a=i*2.4;d=random.random()*.12
    p=Vector((math.cos(a)*d,math.sin(a)*d,0))
    leaf('Bent grass blade',r,p,p+Vector((math.cos(a)*.14,math.sin(a)*.14,random.uniform(.24,.48))),.028,LEAF_LIGHT if i%2 else LEAF,False)
join_meshes(r,'Grass')

r=begin('flower_patch')
for i in range(4):
    a=i*2.4;p=Vector((math.cos(a)*.2,math.sin(a)*.16,0));h=random.uniform(.23,.40)
    curve('Wildflower stalk',r,[p,p+Vector((.02,0,h))],.013,LEAF)
    leaf('Wildflower leaf',r,p+Vector((0,0,.06)),p+Vector((.16,0,.20)),.05,LEAF_LIGHT,False)
    jasmine(r,p+Vector((.02,0,h)),.38,.15)
join_meshes(r,'WildflowerStems')

r=begin('rock_cluster')
for i in range(3):
    o=ico('Weathered stone',r,((i-1)*.18,random.uniform(-.06,.06),.08),(.19,.17,.14),[MORTAR,STONE,CAP][i],1)
    o.rotation_euler.z=random.random()*math.pi
join_meshes(r,'RiverStones')

r=begin('mushroom_patch')
for i in range(3):
    x=(i-1)*.21;h=[.34,.48,.23][i]
    curve('Mushroom stem',r,[(x,0,0),(x+.025,0,h)],.034,IVORY)
    sphere('Mushroom cap',r,(x+.025,0,h),(.16,.16,.07),SHROOM,14,6)
    for j in range(4):
        a=j*2.4
        sphere('Cap fleck',r,(x+.025+math.cos(a)*.075,math.sin(a)*.075,h+.06),(.022,.022,.008),IVORY,8,4)
join_meshes(r,'MushroomCluster')

r=begin('dew_drop');gem=group('FloatingDew',r)
lathe('Dew jewel',gem,[(0,-.23),(.12,-.18),(.19,-.07),(.185,.06),(.12,.20),(.018,.39),(0,.42)],DEW,16)
sphere('Dew glint',gem,(-.063,-.14,.13),(.032,.014,.088),IVORY,10,6)
join_meshes(gem,'DewCrystal')
animate('dew_drop','float',[gem],2.2,lambda t:(setattr(gem.location,'z',math.sin(t*math.tau)*.08),setattr(gem.rotation_euler,'z',math.sin(t*math.tau)*.20)))

r=begin('vine_gate');vines=group('LivingVines',r)
for i in range(8):
    y=(i-3.5)*.43
    curve('Braided living stem',vines,[(0,y,0),(.11,y+.10,.5),(-.08,y-.04,1.02),(.02,y+.04,1.48)],.053,LEAF)
    for z in [.43,.86,1.23]:leaf('Gate leaf',vines,(0,y,z),(.10,y+.26,z+.14),.09,LEAF_LIGHT,False)
join_meshes(vines,'LivingHedge')
animate('vine_gate','open',[vines],.9,lambda t:setattr(vines.location,'z',-1.8*(1-(1-t)**3)))

r=begin('shelter_marker')
bpy.ops.mesh.primitive_circle_add(vertices=64,radius=1.85,fill_type='NGON',location=(0,0,.045))
finish_object(bpy.context.object,'Shelter moss carpet',r,SHADE)
torus('Shelter outer inlay',r,(0,0,.052),1.84,.014,LEAF_LIGHT)
join_meshes(r,'ShelterInlay')

r=begin('ring_marker')
torus('Objective ring',r,(0,0,0),1,.023,GLASS)
join_meshes(r,'RingMarker')

r=begin('petal_particle')
leaf('Loose petal',r,(-.5,0,0),(.5,0,.1),.30,IVORY,False)
join_meshes(r,'PetalParticle')

# Export each authored asset from its own collection, with real Blender NLA clips.
manifest={'generator':'Blender '+bpy.app.version_string,'source':'art-source/garden_library.blend','script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'assets':{}}
for name,asset in assets.items():
    bpy.context.scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT')
    for o in asset['collection'].objects:o.select_set(True)
    bpy.context.view_layer.objects.active=asset['root']
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,
        export_animations=True,export_animation_mode='NLA_TRACKS',export_frame_range=False,
        export_force_sampling=True,export_optimize_animation_size=False,export_materials='EXPORT',export_yup=True,export_extras=True)
    meshes=[o for o in asset['collection'].objects if o.type=='MESH']
    triangles=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes)
    manifest['assets'][name]={'triangles':triangles,'mesh_objects':len(meshes),'clips':asset['clips'],'bytes':(OUT/(name+'.glb')).stat().st_size}
    print('AUTHORED',name,triangles,'triangles',asset['clips'])
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

# An editable, arranged library. Exports above remain at clean local origins.
for i,(name,asset) in enumerate(assets.items()):
    asset['root'].location=((i%5)*5,(i//5)*5,0)
    if name=='garden_base':asset['root'].location=(0,30,0)
# Scene set up for a useful character showcase, not a user's existing document.
scene=bpy.context.scene
scene.world.color=(.12,.12,.12)
scene.render.engine='CYCLES';scene.cycles.samples=32
scene.cycles.use_denoising=True
scene.view_settings.view_transform='AgX'
scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
# Hide all but Jasmin, a beetle and a flower for the optional beauty render.
preview_names={'petit_jasmin','dusk_beetle','sunflower','moonflower','grass_clump','rock_cluster'}
positions={'petit_jasmin':(-.55,0,0),'dusk_beetle':(.72,-.15,0),'sunflower':(-1.4,.45,0),'moonflower':(1.5,.65,0),'grass_clump':(-1.1,-.7,0),'rock_cluster':(1.2,-.55,0)}
collection=bpy.data.collections.new('Studio');scene.collection.children.link(collection)
# A separate scene uses linked copies so the editable library stays neatly arranged.
studio=bpy.data.scenes.new('Character showcase');studio.render.engine='CYCLES';studio.cycles.samples=32;studio.cycles.use_denoising=True
studio.world=bpy.data.worlds.new('Studio world');studio.world.use_nodes=True
studio.world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.20,.18,1)
studio.world.node_tree.nodes['Background'].inputs[1].default_value=.45
studio.view_settings.view_transform='AgX'
studio.render.resolution_x=1400;studio.render.resolution_y=1000;studio.render.resolution_percentage=100
studio_coll=bpy.data.collections.new('Showcase models');studio.collection.children.link(studio_coll)
for name in preview_names:
    source=assets[name]['root'];mapping={}
    for o in assets[name]['collection'].objects:
        dup=o.copy()
        if o.data:dup.data=o.data
        if dup.animation_data:dup.animation_data_clear()
        studio_coll.objects.link(dup);mapping[o]=dup
    for original,dup in mapping.items():
        dup.parent=mapping.get(original.parent)
        if original==source:dup.location=positions[name]
# Smooth studio floor, lighting, camera.
bpy.context.window.scene=studio;collection=studio_coll
floor=cube('Backdrop',None,(0,0,-.12),(200,200,.2),mat('Studio | eucalyptus','677C70'),.02)
bpy.ops.object.camera_add(location=(3.6,-6.5,3.7))
cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.75))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=4.8;studio.camera=cam
for name,loc,power,size in [('Large softbox',(-3,-4,6),500,4),('Warm rim',(3,2,4),650,3)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size
    o=bpy.data.objects.new(name,data);studio.collection.objects.link(o);o.location=loc;o.rotation_euler=(-o.location).to_track_quat('-Z','Y').to_euler()
# Save an actual native Blender source, including meshes, materials, animation and studio.
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'garden_library.blend'))
# The title illustration is rendered from the same authored models, not painted externally.
(ROOT/'game/assets/art').mkdir(exist_ok=True)
studio.render.filepath=str(ROOT/'game/assets/art/title_scene.png')
bpy.ops.render.render(write_still=True)
if '--preview' in sys.argv:
    (SOURCE/'previews').mkdir(exist_ok=True)
    shutil.copy2(studio.render.filepath, SOURCE/'previews/characters.png')
print('BLENDER_ASSET_BUILD_OK',len(assets),'original models')
