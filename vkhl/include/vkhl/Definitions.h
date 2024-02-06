#pragma once

#ifndef VKHL_DEFINITIONS_H
#define VKHL_DEFINITIONS_H

#if defined(VKHL_IMPLEMENTATION) || defined(VKHL_USE_IMPLEMENTATION)
#define VKHL_INLINE

#ifdef VKHL_IMPLEMENTATION
#define VKHL_INLINE_VAR
#else
#define VKHL_INLINE_VAR extern
#endif

#else
#define VKHL_INLINE inline

#define VKHL_INLINE_VAR inline
#endif

#if defined(VKHL_IMPLEMENTATION) || !defined(VKHL_USE_IMPLEMENTATION)
#define VKHL_INCLUDE_IMPLEMENTION
#endif

#endif
