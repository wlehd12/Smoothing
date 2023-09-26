#ifndef __MESH_H__
#define __MESH_H__

#pragma once
#include "Face.h"
#include "Edge.h"
#include "GL\glut.h"

class Mesh
{
public:
	double			_mass = 1.0;
	double			_volume = 0.0;
	Vec3<double>	_minPos;
	Vec3<double>	_maxPos;
	vector<double>	_DihedralAngle;
	vector<Vertex*>	_vertices;
	vector<Face*>	_faces;
	vector<Edge*>	_edges;
public:
	Mesh(char* filename)
	{
		open(filename);
	}
	~Mesh(void) {}
public:
	void	init(void);
	void	reset(void);
	void	open(char* filename);
	void	computeDihedralAngle(void);
	void	computeVolume(void);
	void	computeEdge(void);
	void	computeNormal(void);
	void	moveToCenter(double scaling);
	void	BuildList(void);
	void	Simulation(double dt);
	void	Distance_Constraint(void);
	void	Bending_Constraint(void);
	void	volume_constraints(void);
	void	ExternalForces(double dt);
	void    Force(double dt);
	void	Collision(void);
public:
	void	drawPoint(void);
	void	drawWireframe(void);
	void	drawSurface(bool smooth = false);
};

#endif
