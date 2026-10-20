#pragma once

namespace RenderSys
{

// Points RenderSys at the device Walnut created. Call once the Walnut::Application
// exists and before creating any renderer.
void AttachToWalnut();

} // namespace RenderSys
