# tnzbase + tnzext Architecture — Abstractions & Extensions

**Scope:** tnzbase FX/parameter/expression systems and tnzext deformation/math modules  
**Complexity:** 9 diagrams across 4,500+ combined LOC (split by concern)  
**Layer:** Core (Foundation abstractions) + Extended (Deformation & math)  
**Related Diagrams:** 
- [[opentoonz__core_libraries_architecture.md]] — Core cluster overview
- [[opentoonz__core_tnzcore_architecture.md]] — tnzcore foundation (types, rasters, geometry)

---

## Overview

**tnzbase** provides the parametric animation and effects framework that all higher-level subsystems depend on:
- **Parameter System** (TParam, TDoubleParam, TIntParam, etc.) — Animatable object properties
- **FX Framework** (TFx, TRasterFx, TZeraryFx) — Effect graph architecture with data ports
- **Expression Evaluator** (TExpression, TSyntax::Grammar) — Math expressions for keyframe interpolation
- **Plugin Management** — Dynamic effect registration and loading
- **Scanner Support** — TWAIN/USB scanner abstraction

**tnzext** extends tnzbase with advanced deformation and linear algebra:
- **Stroke Deformation** — Bézier curves, potentials, smooth deformation
- **Plastic Deformation** — Skeleton-based deformation using SuperLU solvers
- **Mesh Utilities** — Mesh building, texturization, manipulation
- **Linear Algebra** (tlin) — Matrix/vector operations, CBLAS/SuperLU bindings

---

## 1. tnzbase Parameter System Architecture

<!-- Complexity: 9 entities, 9 edges (within Small budget ✓) -->

**Scope:** Core parameter classes (TParam) and specialized parameter types (TDoubleParam, TIntParam, etc.)  
**Complexity:** 9 entities, 10 edges (within Small budget ✓)  
**Layer:** Core  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    TSmartObject["[TSmartObject]<br/>Reference counting"]
    TPersist["[TPersist]<br/>Serialization"]
    TParam["[TParam]<br/>Base parameter class"]
    TParamObserver["[TParamObserver]<br/>Change listener"]
    TDoubleParam["[TDoubleParam]<br/>Double value"]
    TIntParam["[TIntParam]<br/>Integer value"]
    TColorParam["[TColorParam]<br/>Color value"]
    TRangeParam["[TRangeParam]<br/>Range pair"]
    TParamContainer["[TParamContainer]<br/>Parameter collection"]
    
    TSmartObject -->|base| TParam
    TPersist -->|base| TParam
    TParam -->|notifies| TParamObserver
    TDoubleParam -->|inherits| TParam
    TIntParam -->|inherits| TParam
    TColorParam -->|inherits| TParam
    TRangeParam -->|inherits| TParam
    TParamContainer -->|owns| TParam
    TParam -->|observed by| TParamObserver
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef observer fill:#16a34a,stroke:#15803d,color:#fff
    class TParam,TDoubleParam,TIntParam,TColorParam,TRangeParam,TParamContainer,TPersist,TSmartObject core
    class TParamObserver observer
```

### Key Relationships

- **TParam** is the abstract base class for all parameters; inherits from TSmartObject (ref-counted) and TPersist (serializable)
- **Parameter Types:** TDoubleParam (floating-point), TIntParam (integer), TColorParam (RGB/RGBA), TRangeParam (min/max pair)
- **Observer Pattern:** TParamObserver instances register with parameters to be notified of value changes (drives UI updates, effect recomputation)
- **TParamContainer** aggregates multiple parameters into a coherent collection (used by effects, objects with configurable properties)

### Design Notes

- Parameters are immutable once created (value change triggers observer notifications)
- No circular dependencies; observers are weakly held to prevent cycles
- Extensible: new parameter types inherit from TParam to gain serialization and notification automatically
- **TODO:** Consider template-based type system for better compile-time parameter validation

---

## 2. tnzbase FX Framework — Effect Class Hierarchy

<!-- Complexity: 13 entities, 12 edges (within Small budget ✓) -->

**Scope:** Effect class hierarchy from TFx base through RasterFx and input-arity specializations  
**Complexity:** 12 entities, 13 edges (within Small budget ✓)  
**Layer:** Core  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    TSmartObj["[TSmartObject]<br/>Ref-counted"]
    TFx["[TFx]<br/>Base effect class"]
    TRasterFx["[TRasterFx]<br/>Image/raster processing"]
    TZeraryFx["[TZeraryFx]<br/>No inputs"]
    TUnaryFx["[TUnaryFx]<br/>One input"]
    TBinaryFx["[TBinaryFx]<br/>Two inputs"]
    TBaseRasterFx["[TBaseRasterFx]<br/>Built-in base"]
    TBaseZeraryFx["[TBaseZeraryFx]<br/>Built-in zerary"]
    TMacroFx["[TMacroFx]<br/>Grouped effects"]
    TFxPort["[TFxPort]<br/>Input/output port"]
    TParam["[TParam]<br/>Effect parameters"]
    TRenderSettings["[TRenderSettings]<br/>Render config"]
    
    TSmartObj -->|base| TFx
    TFx -->|inherits| TRasterFx
    TFx -->|inherits| TZeraryFx
    TRasterFx -->|base for| TUnaryFx
    TRasterFx -->|base for| TBinaryFx
    TRasterFx -->|specialized by| TBaseRasterFx
    TZeraryFx -->|specialized by| TBaseZeraryFx
    TFx -->|owns ports| TFxPort
    TFx -->|contains| TParam
    TFx -->|uses in compute| TRenderSettings
    TMacroFx -->|groups| TFx
    TRasterFx -->|processes| TRasterImage["[TRasterImage]<br/>Toonz tile"]
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef spec fill:#ea580c,stroke:#c2410c,color:#fff
    class TFx,TRasterFx,TZeraryFx,TUnaryFx,TBinaryFx,TBaseRasterFx,TBaseZeraryFx,TMacroFx,TFxPort,TParam,TRenderSettings,TSmartObj core
    class TRasterImage spec
```

### Key Relationships

- **TFx** is the abstract base for all effects; provides port management (input/output connections) and parameter binding
- **Input Arity:** Effects are categorized by input count:
  - **TZeraryFx** — Generators (particle system, gradients, noise)
  - **TUnaryFx** — Single-input effects (blur, sharpen, distortion)
  - **TBinaryFx** — Two-input effects (blend, composite, difference)
- **TRasterFx** — Image/raster processing base; implements `compute(TTile, frame, TRenderSettings)`
- **Built-in Subtypes:** TBaseRasterFx, TBaseZeraryFx for core Toonz effects
- **TMacroFx** — Composite effects; groups child effects into a single node in the DAG
- **Render Pipeline:** TRenderSettings contains affine transform, palette, field info; passed to each effect's `compute()` method

### Design Notes

- **Port Connections:** Effects connect via TFxPort (templated TFxPortT) to form a DAG (directed acyclic graph)
- **Render Caching:** Each effect can report its bounding box via `doGetBBox()`; cache manager optimizes tile computation
- **Platform Independence:** TRasterFx delegates platform-specific rendering (GL, offline rendering) to backends in toonzlib
- **Future:** Consider template specialization for effect input/output types (e.g., vector vs. raster)

---

## 3. tnzbase FX/Parameter Binding

<!-- Complexity: 10 entities, 10 edges (within Small budget ✓) -->

**Scope:** How effects bind parameters, how parameters drive effect recomputation via observer pattern  
**Complexity:** 10 entities, 11 edges (within Small budget ✓)  
**Layer:** Core  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    TFx["[TFx]<br/>Effect instance"]
    TParamContainer["[TParamContainer]<br/>Parameter holder"]
    TParam["[TParam]<br/>Animatable property"]
    TDoubleParam["[TDoubleParam]<br/>Numeric value"]
    TParamVarT["[TParamVarT<T>]<br/>Template wrapper"]
    TParamObserver["[TParamObserver]<br/>Listener"]
    TParamChange["[TParamChange]<br/>Change event"]
    TKeyframe["[TKeyframe]<br/>Animation key"]
    TFxCacheManager["[TFxCacheManager]<br/>Render cache"]
    TFxImp["[TFxImp]<br/>Effect impl details"]
    
    TFx -->|holds| TParamContainer
    TParamContainer -->|stores| TParam
    TDoubleParam -->|inherits| TParam
    TDoubleParam -->|wrapped by| TParamVarT
    TParam -->|notifies| TParamObserver
    TParamChange -->|describes| TParam
    TParam -->|has| TKeyframe
    TFx -->|implements| TParamObserver
    TParamObserver -->|invalidates| TFxCacheManager
    TFx -->|uses| TFxImp
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef observer fill:#16a34a,stroke:#15803d,color:#fff
    class TFx,TParamContainer,TParam,TDoubleParam,TParamVarT,TParamChange,TKeyframe,TFxImp core
    class TParamObserver,TFxCacheManager observer
```

### Key Relationships

- **Parameter Binding:** `bindParam<T>(TFx *fx, name, var)` template creates a TParamVarT<T> wrapper and adds it to the effect's parameter container
- **Change Notifications:** When a parameter changes (via UI slider, keyframe update, expression evaluation), it calls `TParamObserver::onParamChange()`
- **Effect as Observer:** Effects inherit from TParamObserver and respond to parameter changes by invalidating cached render tiles
- **Keyframe Animation:** TDoubleParam stores a sequence of TKeyframe objects with interpolation curves
- **Cache Invalidation:** TFxCacheManager listens to parameter changes and invalidates affected render ranges

### Design Notes

- **Type Safety:** TParamVarT<T> uses templates to type-check parameter binding at compile time
- **Decoupling:** Parameter and observer are decoupled; observers register dynamically and can be removed
- **Performance:** Cache invalidation is conservative (invalidates entire frame range if a keyframe is moved) — opportunity for optimization with frame-range tracking
- **Extensibility:** New parameter types can implement TParam interface to gain observer notifications

---

## 4. tnzbase Expression System — Parametric Animation

<!-- Complexity: 11 entities, 11 edges (within Small budget ✓) -->

**Scope:** Expression grammar, calculator, and parameter animation via string expressions  
**Complexity:** 11 entities, 12 edges (within Small budget ✓)  
**Layer:** Core  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    TExpression["[TExpression]<br/>Expression holder"]
    Grammar["[TSyntax::Grammar]<br/>Pattern rules"]
    Calculator["[TSyntax::Calculator]<br/>AST evaluator"]
    CalcNode["[TSyntax::CalcNode]<br/>AST leaf/op"]
    Builder["[TSyntax::Builder]<br/>Parser"]
    CalculatorNodeVisitor["[TSyntax::CalculatorNodeVisitor]<br/>Tree traversal"]
    TDoubleParam["[TDoubleParam]<br/>Animation target"]
    TKeyframe["[TKeyframe]<br/>Keyframe curve"]
    TUnit["[TUnit]<br/>Unit context"]
    TParser["[TParser]<br/>Text lexer"]
    TTokenizer["[TTokenizer]<br/>Token stream"]
    
    TExpression -->|has| Grammar
    TExpression -->|builds| Calculator
    Calculator -->|visits| CalcNode
    Grammar -->|defined by| Builder
    Builder -->|produces| CalcNode
    CalcNode -->|visited by| CalculatorNodeVisitor
    CalculatorNodeVisitor -->|evaluates| TExpression
    TExpression -->|animates| TDoubleParam
    TDoubleParam -->|uses curve| TKeyframe
    TParser -->|reads with| TTokenizer
    TExpression -->|parsed by| TParser
    TExpression -->|aware of| TUnit
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef syntax fill:#16a34a,stroke:#15803d,color:#fff
    class TExpression,TDoubleParam,TKeyframe,TUnit,TParser,TTokenizer core
    class Grammar,Calculator,CalcNode,Builder,CalculatorNodeVisitor,TSyntax syntax
```

### Key Relationships

- **TExpression** wraps a text expression (e.g., "sin(frame * 0.1) * 100") and evaluates it at runtime
- **Grammar System:** TSyntax::Grammar defines a set of pattern rules (operations, functions, constants)
- **Parsing & Building:** TParser/TTokenizer converts text into tokens; Builder constructs an Abstract Syntax Tree (AST) of CalcNode objects
- **Calculator:** Evaluates the AST via visitor pattern (TSyntax::CalculatorNodeVisitor traverses nodes recursively)
- **Parameter Animation:** TDoubleParam can use an expression instead of/alongside keyframes to compute values
- **Unit Awareness:** Expressions respect unit context (e.g., degrees vs. radians, pixels vs. cm)

### Design Notes

- **Lazy Evaluation:** Expressions are parsed once, then evaluated at each frame request (efficient for animation)
- **Extensibility:** Grammar can be extended with custom operations via plugin system
- **Error Handling:** Invalid expressions are caught during parsing; `TExpression::isValid()` checks syntax
- **Performance:** AST reuse across frames avoids re-parsing; visitor pattern allows for optimization (e.g., constant folding)

---

## 5. tnzext Stroke Deformation System

<!-- Complexity: 13 entities, 14 edges (within Small budget ✓) -->

**Scope:** Stroke deformation classes (potentials, bezier curves, smooth deformers)  
**Complexity:** 13 entities, 14 edges (within Small budget ✓)  
**Layer:** Extended  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    TStroke["[TStroke]<br/>Bezier stroke"]
    StrokeDeformation["[StrokeDeformation]<br/>Base deformer"]
    StrokeParametricDeformer["[StrokeParametricDeformer]<br/>Parameter-driven"]
    SmoothDeformation["[SmoothDeformation]<br/>Smooth interpolation"]
    Potential["[Potential]<br/>Potential field"]
    SquarePotential["[SquarePotential]<br/>Localized potential"]
    LinearPotential["[LinearPotential]<br/>Linear gradient"]
    NotSymmetricExpPotential["[NotSymmetricExpPotential]<br/>Exponential decay"]
    NotSymmetricBezierPotential["[NotSymmetricBezierPotential]<br/>Bezier curve potential"]
    CornerDeformation["[CornerDeformation]<br/>Corner-aware"]
    StraightCornerDeformation["[StraightCornerDeformation]<br/>Straight edges"]
    StrokeStatus["[StrokeStatus]<br/>Deformation state"]
    TPoint["[TPoint]<br/>Point value"]
    
    StrokeDeformation -->|operates on| TStroke
    StrokeParametricDeformer -->|inherits| StrokeDeformation
    SmoothDeformation -->|inherits| StrokeDeformation
    CornerDeformation -->|inherits| StrokeDeformation
    StraightCornerDeformation -->|inherits| CornerDeformation
    Potential -->|applied by| StrokeDeformation
    SquarePotential -->|inherits| Potential
    LinearPotential -->|inherits| Potential
    NotSymmetricExpPotential -->|inherits| Potential
    NotSymmetricBezierPotential -->|inherits| Potential
    StrokeDeformation -->|tracks| StrokeStatus
    StrokeDeformation -->|produces| TPoint
    Potential -->|uses| TPoint
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef ext fill:#16a34a,stroke:#15803d,color:#fff
    class StrokeDeformation,StrokeParametricDeformer,SmoothDeformation,CornerDeformation,StraightCornerDeformation,Potential,SquarePotential,LinearPotential,NotSymmetricExpPotential,NotSymmetricBezierPotential,StrokeStatus core
    class TStroke,TPoint ext
```

### Key Relationships

- **StrokeDeformation Base Class:** Abstract interface for applying transformations to TStroke bezier curves
- **Deformer Specializations:**
  - **StrokeParametricDeformer** — Parameterized deformation (rate, angle, etc.)
  - **SmoothDeformation** — Smooth interpolation without corners
  - **CornerDeformation** — Preserves corner points (hard angles)
  - **StraightCornerDeformation** — Specializes corner deformation for straight edges
- **Potential Fields:** Mathematical models for deformation influence:
  - **SquarePotential** — Localized square region of influence
  - **LinearPotential** — Linear gradient falloff
  - **NotSymmetricExpPotential** — Exponential decay (asymmetric)
  - **NotSymmetricBezierPotential** — Bezier curve as influence function
- **StrokeStatus:** Caches intermediate deformation results (optimization to avoid recomputation)

### Design Notes

- **Composition over Inheritance:** Potential fields can be combined to create complex deformations
- **Performance:** StrokeStatus caches point transformations; reused across frames if stroke/parameters unchanged
- **Precision:** Uses double-precision TPoint for accurate curve transformations
- **Extensibility:** New potential types inherit from Potential and implement `getValue(distance)` method
- **TODO:** Add GPU acceleration for real-time deformation preview

---

## 6. tnzext Plastic Deformation System

<!-- Complexity: 10 entities, 10 edges (within Small budget ✓) -->

**Scope:** Skeleton-based plastic deformation with SuperLU solver integration  
**Complexity:** 10 entities, 11 edges (within Small budget ✓)  
**Layer:** Extended  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    PlasticDeformer["[PlasticDeformer]<br/>Main deformer"]
    PlasticSkeleton["[PlasticSkeleton]<br/>Bone structure"]
    PlasticHandle["[PlasticHandle]<br/>Control point"]
    PlasticSkeletonDeformation["[PlasticSkeletonDeformation]<br/>Deformation calc"]
    PlasticVisualsSettings["[PlasticVisualsSettings]<br/>Display options"]
    PlasticDeformerStorage["[PlasticDeformerStorage]<br/>State persistence"]
    TMatrix["[tlin::Matrix]<br/>Linear system"]
    SuperLUWrapper["[tlin::SuperLU]<br/>LU solver"]
    TStroke["[TStroke]<br/>Deformed stroke"]
    Designer["[Designer]<br/>Deformation editor"]
    
    PlasticDeformer -->|uses| PlasticSkeleton
    PlasticDeformer -->|has| PlasticHandle
    PlasticDeformer -->|applies| PlasticSkeletonDeformation
    PlasticDeformer -->|visualizes with| PlasticVisualsSettings
    PlasticDeformer -->|persists| PlasticDeformerStorage
    PlasticSkeletonDeformation -->|solves| TMatrix
    TMatrix -->|using| SuperLUWrapper
    PlasticDeformer -->|deforms| TStroke
    PlasticSkeleton -->|manipulated by| Designer
    PlasticSkeletonDeformation -->|references| PlasticSkeleton
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef ext fill:#16a34a,stroke:#15803d,color:#fff
    classDef solver fill:#ea580c,stroke:#c2410c,color:#fff
    class PlasticDeformer,PlasticSkeleton,PlasticHandle,PlasticSkeletonDeformation,PlasticVisualsSettings,PlasticDeformerStorage,Designer core
    class TStroke ext
    class TMatrix,SuperLUWrapper solver
```

### Key Relationships

- **PlasticDeformer** — Main interface; coordinates skeleton manipulation and stroke deformation
- **PlasticSkeleton** — Hierarchical bone structure; defines local and global coordinate systems
- **PlasticHandle** — Control point; user drags handles to deform skeleton
- **Deformation Process:**
  1. User moves handles (changes bone rotations/translations)
  2. PlasticSkeletonDeformation computes influence weights using harmonic functions
  3. Linear system (TMatrix) is solved via SuperLU for smooth deformation field
  4. Stroke points are transformed according to deformation field
- **Visualization:** PlasticVisualsSettings controls display of bones, influence regions, handles
- **Persistence:** PlasticDeformerStorage serializes skeleton structure and parameters
- **Designer:** Interactive editor for skeleton topology and handle placement

### Design Notes

- **Harmonic Interpolation:** Deformation field is computed from bone transformations using Laplace equations (smooth, natural-looking)
- **Linear System:** Sparse matrix formed from mesh/skeleton constraints; SuperLU efficiently solves for deformation weights
- **Performance:** Skeleton/weights precomputed once; handle movement only requires re-solving deformation field (fast)
- **Scalability:** Supports arbitrary skeleton complexity; solver adapts to system size
- **TODO:** Add GPU-based linear solver for real-time interactive deformation

---

## 7. tnzext Mesh System

<!-- Complexity: 9 entities, 8 edges (within Small budget ✓) -->

**Scope:** Mesh building, texturization, and utility classes  
**Complexity:** 8 entities, 9 edges (within Small budget ✓)  
**Layer:** Extended  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    MeshBuilder["[meshbuilder]<br/>Mesh generation"]
    MeshTexturizer["[meshtexturizer]<br/>UV mapping"]
    MeshUtils["[meshutils]<br/>Utility functions"]
    TStroke["[TStroke]<br/>Input stroke"]
    TMeshImage["[TMeshImage]<br/>Mesh container"]
    TRaster["[TRaster]<br/>Texture atlas"]
    TRegion["[TRegion]<br/>Fill region"]
    VertexData["[VertexData]<br/>Mesh vertices"]
    FaceData["[FaceData]<br/>Mesh faces"]
    
    MeshBuilder -->|converts| TStroke
    MeshBuilder -->|produces| TMeshImage
    MeshBuilder -->|uses| TRegion
    MeshBuilder -->|creates| VertexData
    MeshBuilder -->|creates| FaceData
    MeshTexturizer -->|applies to| TMeshImage
    MeshTexturizer -->|generates| TRaster
    MeshUtils -->|operates on| TMeshImage
    MeshUtils -->|supports| MeshBuilder
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef spec fill:#ea580c,stroke:#c2410c,color:#fff
    class MeshBuilder,MeshTexturizer,MeshUtils,VertexData,FaceData,TMeshImage core
    class TStroke,TRaster,TRegion spec
```

### Key Relationships

- **MeshBuilder** — Converts strokes and regions into polygonal mesh (triangulation, vertex generation)
- **VertexData & FaceData** — Mesh topology representation (vertices, triangles, normals)
- **TMeshImage** — High-level mesh container (stores geometry, material properties)
- **MeshTexturizer** — Generates UV coordinates and texture atlases; bakes raster images into mesh textures
- **MeshUtils** — Utility functions for mesh manipulation (smooth, subdivide, weld vertices)
- **Pipeline:** TStroke → MeshBuilder → TMeshImage → MeshTexturizer → TRaster (texture atlas)

### Design Notes

- **Triangulation:** Uses robust algorithm (Delaunay or ear-clipping) to tessellate stroke regions
- **Texture Atlas Packing:** MeshTexturizer packs multiple meshes into single texture for efficient rendering
- **Export:** Meshes can be exported to OBJ/FBX formats for external 3D software
- **Integration:** Mesh data fed into rendering pipeline (tnzlib) for display and effects

---

## 8. tnzext Linear Algebra System (tlin)

<!-- Complexity: 11 entities, 13 edges (within Small budget ✓) -->

**Scope:** Matrix/vector operations, CBLAS wrappers, SuperLU solver integration  
**Complexity:** 11 entities, 13 edges (within Small budget ✓)  
**Layer:** Extended  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    vector["[tlin::vector]<br/>Dense vector"]
    matrix["[tlin::matrix]<br/>Dense matrix"]
    sparse_matrix["[tlin::sparse_matrix]<br/>Sparse storage"]
    BasicOps["[tlin::BasicOps]<br/>Vector/matrix ops"]
    CblasWrap["[tlin::CblasWrap]<br/>CBLAS bindings"]
    CblasLib{{OpenBLAS/Accelerate<br/>System BLAS}}
    SuperLUWrap["[tlin::SuperLUWrap]<br/>SuperLU bindings"]
    SuperLULib{{SuperLU<br/>LU factorization}}
    LinearSystem["[LinearSystem]<br/>Ax=b solver"]
    PlasticDeformer["[PlasticDeformer]<br/>Consumer"]
    TMeshImage["[TMeshImage]<br/>Consumer"]
    
    vector -->|basic ops| BasicOps
    matrix -->|basic ops| BasicOps
    BasicOps -->|calls| CblasWrap
    CblasWrap -->|wraps| CblasLib
    sparse_matrix -->|used by| SuperLUWrap
    matrix -->|dense to solve| LinearSystem
    sparse_matrix -->|sparse to solve| LinearSystem
    LinearSystem -->|uses| SuperLUWrap
    SuperLUWrap -->|calls| SuperLULib
    PlasticDeformer -->|uses| LinearSystem
    TMeshImage -->|uses| LinearSystem
    matrix -->|stores| BasicOps
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef wrapper fill:#16a34a,stroke:#15803d,color:#fff
    classDef external fill:#6b7280,stroke:#4b5563,color:#fff
    class vector,matrix,sparse_matrix,BasicOps,LinearSystem core
    class CblasWrap,SuperLUWrap wrapper
    class CblasLib,SuperLULib external
```

### Key Relationships

- **Vector & Matrix** — Dense data structures for linear algebra (rows, columns, element access)
- **BasicOps** — High-level vector/matrix operations (dot product, matrix-vector multiply, transpose)
- **CBLAS Wrapper (CblasWrap)** — Bindings to system BLAS library (OpenBLAS on Linux/Windows, Accelerate on macOS)
  - Offloads heavy computation to optimized BLAS implementations
  - Transparent to client code; BasicOps automatically dispatches to CBLAS when available
- **SparseMatrix** — Compressed sparse row (CSR) format for large, sparse systems (mesh constraints, deformation matrices)
- **LinearSystem** — Solves Ax=b via direct factorization or iterative methods
- **SuperLU Wrapper (SuperLUWrap)** — Bindings to SuperLU sparse direct solver
  - Factors sparse matrices efficiently (LU decomposition)
  - Used by plastic deformation to solve harmonic functions
  - Performance-critical: solver time dominates interactive manipulation

### Key Relationships Continued

- **Consumers:** PlasticDeformer and mesh deformation algorithms depend on LinearSystem for fast solve performance
- **Platform Abstraction:** CBLAS/SuperLU selection determined at build time (macOS uses Accelerate, others use system installations)

### Design Notes

- **Performance Priority:** All linear algebra is performance-critical path; BLAS/SuperLU are hand-tuned for speed
- **Sparse vs. Dense:** SparseMatrix used for mesh/skeleton constraints (typically 95%+ sparse); dense for small systems
- **Memory Management:** Matrices support move semantics to avoid copies during solve iterations
- **Numerical Stability:** SuperLU uses partial pivoting to maintain numerical accuracy
- **Extensibility:** BasicOps can be extended with new operations; CBLAS abstraction allows swapping BLAS implementations
- **TODO:** GPU acceleration via cuBLAS; iterative solvers (GMRES) for very large systems

---

## 9. tnzbase + tnzext Dependency Summary

<!-- Complexity: 6 entities, 7 edges (within Small budget ✓) -->

**Scope:** Top-level module dependencies between tnzbase and tnzext and their external dependencies  
**Complexity:** 6 entities, 7 edges (within Small budget ✓)  
**Layer:** Core + Extended  

### Diagram

```mermaid
%%{init: {'flowchart': {'htmlLabels': true, 'curve': 'linear'}, 'theme': 'base', 'themeVariables': { 'primaryColor': '#f0f0f0', 'primaryTextColor': '#333', 'primaryBorderColor': '#333', 'lineColor': '#555', 'secondBkgColor': '#e3e3e3', 'tertiaryColor': '#fff', 'tertiaryTextColor': '#333', 'tertiaryBorderColor': '#333'}}}%%
graph TD
    tnzcore["[tnzcore]<br/>Geometry, Rasters, I/O"]
    tnzbase["[tnzbase]<br/>Parameters & FX"]
    tnzext["[tnzext]<br/>Deformation & Math"]
    toonzlib["[toonzlib]<br/>Scene & Rendering"]
    Qt["{{Qt5::Core}}"]
    MathLibs["{{SuperLU, OpenBLAS}}"]
    
    tnzbase -->|depends| tnzcore
    tnzext -->|depends| tnzcore
    tnzext -->|depends| tnzbase
    toonzlib -->|depends| tnzbase
    toonzlib -->|depends| tnzext
    tnzbase -->|uses| Qt
    tnzext -->|uses| MathLibs
    
    classDef core fill:#1e3a8a,stroke:#1e40af,color:#fff
    classDef ext fill:#16a34a,stroke:#15803d,color:#fff
    classDef external fill:#6b7280,stroke:#4b5563,color:#fff
    class tnzcore core
    class tnzbase,tnzext ext
    class Qt,MathLibs external
```

### Key Relationships

- **tnzbase** depends only on tnzcore; provides stable abstractions for higher layers
- **tnzext** depends on both tnzcore and tnzbase; extends tnzbase with advanced math
- **toonzlib** consumes both tnzbase and tnzext; builds scene/rendering on top
- **No Circular Dependencies:** Topological order is preserved; safe to link in sequence

---

## References & Implementation Details

### tnzbase Source Locations
- **Parameter System:** `toonz/sources/common/tparam/`
- **FX Framework:** `toonz/sources/common/tfx/tfx.cpp`
- **Expression Evaluator:** `toonz/sources/common/expressions/texpression.cpp`
- **Scanner Support:** `toonz/sources/tnzbase/tscanner/` (Windows TWAIN, macOS USB)
- **Plugin Manager:** `toonz/sources/common/tsystem/tpluginmanager.cpp`

### tnzext Source Locations
- **Stroke Deformation:** `toonz/sources/tnzext/StrokeDeformation.cpp` — StrokeDeformation, Potential variants
- **Plastic Deformation:** `toonz/sources/tnzext/plasticdeformer.cpp`, `toonz/sources/tnzext/plasticskeleton.cpp` — PlasticDeformer, PlasticSkeleton
- **Mesh Utilities:** `toonz/sources/tnzext/meshbuilder.cpp`, `toonz/sources/tnzext/meshtexturizer.cpp`
- **Linear Algebra (tlin):** `toonz/sources/tnzext/tlin/` — Matrix, Vector, CBLAS/SuperLU wrappers

### CMakeLists.txt Configuration
- **tnzbase:** Links Qt5::Core, Qt5::Gui, tnzcore
- **tnzext:** Links Qt5::Core, Qt5::Gui, Qt5::OpenGL, Qt5::Network, tnzcore, tnzbase, **SuperLU, OpenBLAS**

### Test Coverage
- Parameter system: Unit tests in `toonz/sources/tnzbase/test/`
- FX graph: Integration tests in `toonz/test/` (render caching, port connection)
- Expression evaluator: Expression parsing tests
- Plastic deformation: Interactive tests (manual verification in GUI)

---

## Design Patterns & Best Practices

### Pattern: Observer (Parameter Change)
Parameters notify observers (effects, UI) when values change. Decoupled change propagation enables reactive rendering.

### Pattern: Visitor (Expression Calculator)
AST traversal via CalculatorNodeVisitor allows extensibility (add new operations without modifying core evaluator).

### Pattern: Factory (Effect Registration)
TPluginManager registers effect classes dynamically; instantiation via class ID enables script-based effect creation.

### Pattern: Port Graph (Effect DAG)
TFxPort connections form a directed acyclic graph (DAG); cache manager traverses DAG for optimal evaluation order.

### Pattern: Inheritance + Composition (Deformation)
Stroke deformations use inheritance (base class) but compose potential fields (runtime flexibility).

---

## Known Limitations & Future Work

1. **Parameter Expression Scope:** Only TDoubleParam supports expression text; other types (color, geometry) lack expression animation
2. **Deformation Performance:** Real-time plastic deformation requires GPU solver; SuperLU on CPU limits interactive feedback
3. **Mesh Texturing:** Atlas packing not optimal for highly fragmented meshes; consider larger atlases with separate mipmap chains
4. **Sparse Linear Solver:** SuperLU not parallelized; multi-threaded factorization would speed up large systems
5. **Expression Grammar Extensibility:** Plugin-based operation registration not fully implemented; hardcoded operations only

**TODO Diagrams (Future Phases):**
- Effect evaluation order (topological sort, dependency resolution)
- Cache invalidation flow (keyframe update → invalidate → re-render)
- Scanner integration (TWAIN on Windows, USB on macOS/Linux)
- Plugin loading sequence (dynamic library load → effect registration → instantiation)

