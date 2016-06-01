#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

//DOMs
static const std::string X3D_LABEL = "X3D";
static const std::string X3D_LABEL_APP = "Appearance";
static const std::string X3D_LABEL_COMPOSED_SHADER = "ComposedShader";
static const std::string X3D_LABEL_EXT_GEO = "ExternalGeometry";
static const std::string X3D_LABEL_FIELD = "Field";
static const std::string X3D_LABEL_GLTF = "glTF";
static const std::string X3D_LABEL_GROUP = "Group";
static const std::string X3D_LABEL_IMG_TEXTURE = "ImageTexture";
static const std::string X3D_LABEL_INLINE = "Inline";
static const std::string X3D_LABEL_MAT = "Material";
static const std::string X3D_LABEL_MAT_TRANS = "MatrixTransform";
static const std::string X3D_LABEL_MULTIPART = "MultiPart";
static const std::string X3D_LABEL_PLANE = "Plane";
static const std::string X3D_LABEL_SCENE = "Scene";
static const std::string X3D_LABEL_SHADER_PART = "ShaderPart";
static const std::string X3D_LABEL_SHAPE = "Shape";
static const std::string X3D_LABEL_TEXT_PROP = "TextureProperties";
static const std::string X3D_LABEL_TRANS = "Transform";
static const std::string X3D_LABEL_TWOSIDEMAT = "TwoSidedMaterial";
static const std::string X3D_LABEL_VIEWPOINT = "Viewpoint";

//Attribute Names
static const std::string X3D_ATTR_BIND = "bind";
static const std::string X3D_ATTR_BBOX_CENTRE = "bboxCenter";
static const std::string X3D_ATTR_BBOX_SIZE = "bboxSize";
static const std::string X3D_ATTR_CENTRE = "center";
static const std::string X3D_ATTR_COL_DIFFUSE = "diffuseColor";
static const std::string X3D_ATTR_COL_BK_DIFFUSE = "backDiffuseColor";
static const std::string X3D_ATTR_COL_EMISSIVE = "emissiveColor";
static const std::string X3D_ATTR_COL_BK_EMISSIVE = "backEmissiveColor";
static const std::string X3D_ATTR_COL_SPECULAR = "specularColor";
static const std::string X3D_ATTR_COL_BK_SPECULAR = "backSpecularColor";
static const std::string X3D_ATTR_DEF = "def";
static const std::string X3D_ATTR_DO_PICK_PASS = "dopickpass";
static const std::string X3D_ATTR_FOV = "fieldOfView";
static const std::string X3D_ATTR_GEN_MIPMAPS = "generateMipMaps";
static const std::string X3D_ATTR_ID = "id";
static const std::string X3D_ATTR_INVISIBLE = "invisible";
static const std::string X3D_ATTR_LIT = "lit";
static const std::string X3D_ATTR_MAT = "matrix";
static const std::string X3D_ATTR_NAME = "name";
static const std::string X3D_ATTR_NAMESPACE = "nameSpaceName";
static const std::string X3D_ATTR_ON_MOUSE_MOVE = "onmousemove";
static const std::string X3D_ATTR_ON_MOUSE_OVER = "onmouseover";
static const std::string X3D_ATTR_ON_CLICK = "onclick";
static const std::string X3D_ATTR_ON_LOAD = "onload";
static const std::string X3D_ATTR_SCALE = "scale";
static const std::string X3D_ATTR_SIZE = "size";
static const std::string X3D_ATTR_ROTATION = "rotation";
static const std::string X3D_ATTR_TRANSLATION = "translation";
static const std::string X3D_ATTR_TRANSPARENCY = "transparency";
static const std::string X3D_ATTR_BK_TRANSPARENCY = "backTransparency";
static const std::string X3D_ATTR_TYPE = "type";
static const std::string X3D_ATTR_ORIENTATION = "orientation";
static const std::string X3D_ATTR_POS = "position";
static const std::string X3D_ATTR_RENDER = "render";
static const std::string X3D_ATTR_ROT_CENTRE = "centerOfRotation";
static const std::string X3D_ATTR_SHININESS = "shininess";
static const std::string X3D_ATTR_BK_SHININESS = "backShininess";
static const std::string X3D_ATTR_SOLID = "solid";
static const std::string X3D_ATTR_URL = "url";
static const std::string X3D_ATTR_URL_IDMAP = "urlIDMap";
static const std::string X3D_ATTR_VALUE = "value";
static const std::string X3D_ATTR_VALUES = "values";
static const std::string X3D_ATTR_XMLNS = "xmlns";
static const std::string X3D_ATTR_XORIGIN = "crossOrigin";
static const std::string X3D_ATTR_ZFAR = "zFar";
static const std::string X3D_ATTR_ZNEAR = "zNear";

//Values
static const std::string X3D_SPEC_URL = "http://www.web3d.org/specification/x3d-namespace";
static const std::string X3D_ON_CLICK = "clickObject(event);";
static const std::string X3D_ON_LOAD = "onLoaded(event);";
static const std::string X3D_ON_MOUSE_MOVE = "onMouseOver(event);";
static const std::string X3D_ON_MOUSE_OVER = "onMouseMove(event);";
static const std::string X3D_USE_CRED = "use-credentials";
static const float       X3D_DEFAULT_FOV = 0.25 * M_PI;

static const float      GOOGLE_TILE_SIZE = 640;