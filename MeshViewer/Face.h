#ifndef __FACE_H__
#define __FACE_H__

#pragma once
#include "Vertex.h"

class Face
{
public:
	int				_index;
	Vec3<double>	_normal;
	vector<Vertex*>	_vertices;
	vector<Edge*>	_edges;
public:
	Face(int index, Vertex* v0, Vertex* v1, Vertex* v2)
	{
		_index = index;
		_vertices.push_back(v0);
		_vertices.push_back(v1);
		_vertices.push_back(v2);
	}
	~Face(void) {}
public:
	inline Vertex* v(int i) { return _vertices[i]; }
	int getIndex(Vertex *v);
	int hasVertex(Vertex *nv);
	int hasEdge(Edge *ne);
	Vertex *getOtherVertex(Edge *e);
};

#endif