## BIG TODO: basic ui

## SMALL TODOS: (ordered)

1. renderer refactor
   - store static models and shaders in RenderData
   - store RenderData on the Renderer
   - make asset handles their own distinct type
   - get rid of the weird AssetType thingies
   - get rid of render pass types

---

## NEXT BIG TODOS: (unordered)

- change the interaction mechanic, interaction by key is stupid for my game,
  - i need more interaction by mouse click and the interaction radius would be on the player and
  - not on the interactable entities

- placing/destroying something on the ground
- debug tools
  - map/entity/keymap editor
  - time scaling
  - system for in game tests
- profiler
- inventory system

- more on lighting
  - pcf or other technique for better shadow mapping
  - gamma correction
  - hdr?
  - bloom?
  - ssao
  - deferred shading?
  - somehow get rid of the visible rings coming from the light bulb
  - directional light with shadow mapping

- better collision detection

- multithreading
  - if i find the need for it

- get rid of sdl3(big future)

- read the projection [matrix article](https://www.songho.ca/opengl/gl_projectionmatrix.html#fov)
- read the look at [matrix article](https://morning-flow.com/2023/02/06/the-math-behind-the-lookat-transform/)
