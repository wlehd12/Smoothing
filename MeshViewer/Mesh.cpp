#include "Mesh.h"


void Mesh::open(char* filename)
{
	FILE* fp;
	char header[256] = { 0 };
	double pos[3];
	int index = 0;
	_minPos.Set(100000.0);
	_maxPos.Set(-100000.0);

	fopen_s(&fp, filename, "r");
	while (fscanf(fp, "%s %lf %lf %lf", header, &pos[0], &pos[1], &pos[2]) != EOF) {
		if (header[0] == 'v' && header[1] == NULL) {
			for (int i = 0; i < 3; i++) {
				if (_minPos[i] > pos[i]) _minPos[i] = pos[i];
				if (_maxPos[i] < pos[i]) _maxPos[i] = pos[i];
			}
			_vertices.push_back(new Vertex(index++, Vec3<double>(pos[0], pos[1], pos[2])));
		}
	}

	index = 0;
	int indices[3];
	fseek(fp, 0, SEEK_SET);
	while (fscanf(fp, "%s %d %d %d", header, &indices[0], &indices[1], &indices[2]) != EOF) {
		if (header[0] == 'f' && header[1] == NULL) {
			auto v0 = _vertices[indices[0] - 1];
			auto v1 = _vertices[indices[1] - 1];
			auto v2 = _vertices[indices[2] - 1];
			_faces.push_back(new Face(index++,  v0, v1, v2));
		}
	}
	fclose(fp);
	moveToCenter(5.0);
	BuildList();
	computeNormal();
	computeDihedralAngle();
	computeVolume();

	printf("Vertices num. : %d\n", _vertices.size());
	printf("Faces num. : %d\n", _faces.size());
}

void Mesh::init(void) 
{
	for (auto v : _vertices) {
		v->_flag = false;
	}
}

void Mesh::BuildList(void)
{
	// v-f
	for (auto f : _faces) {
		for (auto fv : f->_vertices) {
			fv->_nbFaces.push_back(f);
		}
	}

	// v-v
	for (auto v : _vertices) {
		for (auto nf : v->_nbFaces) {
			int index = nf->getIndex(v);
			v->_nbVertices.push_back(nf->v((index + 1) % 3));
			v->_nbVertices.push_back(nf->v((index + 3 - 1) % 3));
		}
	}

	computeEdge();
	// e-f
	for (auto e : _edges) {
		auto v = e->v(0);
		for (auto nf : v->_nbFaces) {
			if (nf->hasVertex(e->v(0)) && nf->hasVertex(e->v(1))) {
				if (!nf->hasEdge(e)) {
					nf->_edges.push_back(e);
				}
				if (!e->hasFace(nf)) {
					e->_nbFaces.push_back(nf);
				}
			}
		}
	}
}

void Mesh::computeVolume(void)
{
	for (auto f : _faces) {
		auto p0 = f->v(0)->_pos0;
		auto p1 = f->v(1)->_pos0;
		auto p2 = f->v(2)->_pos0;
		if (((p1 - p0).Cross(p2 - p0)).Dot(p0) > 0)
		{
			p1 = f->v(2)->_pos0;
			p2 = f->v(1)->_pos0;
		}
		_volume += (p0.Cross(p1)).Dot(p2);
	}
}

void Mesh::computeEdge(void)
{
	int index = 0;
	for (auto v : _vertices) {
		for (auto nv : v->_nbVertices) {
			if (!nv->_flag) {
				_edges.push_back(new Edge(index++, v, nv));
			}
		}
		v->_flag = true;
	}
	init();
	printf("Edges num. : %d\n", _edges.size());
}

void Mesh::computeNormal(void)
{
	// face normal
	for (auto f : _faces) {
		auto v0 = f->v(1)->_pos0 - f->v(0)->_pos0;
		auto v1 = f->v(2)->_pos0 - f->v(0)->_pos0;
		f->_normal = v0.Cross(v1);
		f->_normal.Normalize();
	}
	
	// vertex normal
	for (auto v : _vertices) {
		v->_normal.Clear();
		for (auto nf : v->_nbFaces) {
			v->_normal += nf->_normal;
		}
		v->_normal /= (double)v->_nbFaces.size();
	}	
}


void Mesh::computeDihedralAngle(void)
{
	for (auto e : _edges) {
		if (e->_nbFaces.size() == 2) {
			auto p0 = e->_nbFaces[0]->getOtherVertex(e)->_pos0;
			auto p1 = e->_nbFaces[1]->getOtherVertex(e)->_pos0;
			auto p2 = e->v(0)->_pos0;
			auto p3 = e->v(1)->_pos0;
			Vec3<double> _edge = p3 - p2;
			double length = _edge.Length();
			double invLength = 1.0 / length;
			Vec3<double> n1 = (p2 - p0).Cross(p3 - p0);
			Vec3<double> n2 = (p3 - p1).Cross(p2 - p1);
			n1 /= n1.LengthSquared();
			n2 /= n2.LengthSquared();

			Vec3<double> d0 = n1 * length;
			Vec3<double> d1 = n2 * length;
			Vec3<double> d2 = n1 * ((p0 - p3).Dot(_edge) * invLength) + n2 * ((p1 - p3).Dot(_edge) * invLength);
			Vec3<double> d3 = n1 * ((p2 - p0).Dot(_edge) * invLength) + n2 * ((p2 - p1).Dot(_edge) * invLength);

			n1.Normalize();
			n2.Normalize();
			double dot = n1.Dot(n2);

			if (dot < -1.0) {
				dot = -1.0;
			}
			if (dot > 1.0) {
				dot = 1.0;
			}

			double restAngle = acos(dot);
			_DihedralAngle.push_back(restAngle);
		}
	}
	printf("DihedralAngle Size : %d\n", _DihedralAngle.size());

}

void Mesh::Force(double dt)
{
	Vec3<double> upforce(0.0, 0.02, 0.0);
	double damping = 0.99;
	for (auto f : _faces) {
		auto p0 = f->v(0)->_newpos;
		auto p1 = f->v(1)->_newpos;
		auto p2 = f->v(2)->_newpos;
		auto normal = (p1 - p0).Cross(p2 - p0);
		normal.Normalize();
		auto force = normal * dt;
		f->v(0)->_vel += (force + upforce) * damping;
		f->v(1)->_vel += (force + upforce) * damping;
		f->v(2)->_vel += (force + upforce) * damping;
	}
}

void Mesh::ExternalForces(double dt)
{
	Vec3<double> gravity(0.0, -9.8, 0.0);
	double damping = 0.99;
	for (auto v : _vertices) {
		v->_vel += gravity * dt *  _mass;
		v->_vel *= damping;
		v->_newpos = v->_pos + (v->_vel * dt);
	}

}

void Mesh::Collision(void)
{
	for (auto v : _vertices) {
		if (v->_newpos.y() < -10)
			if (v->_newpos.x() < 10 && v->_newpos.x() > -10)
				if (v->_newpos.z() < 10 && v->_newpos.z() > -10)
					v->_pos.y(-10);
	}
}


void Mesh::Distance_Constraint(void)
{
	for (auto e : _edges) {
		double c_p1p2 = (e->v(0)->_newpos - e->v(1)->_newpos).Length() - (e->v(0)->_pos0 - e->v(1)->_pos0).Length();
		Vec3<double> dp1 = (e->v(0)->_newpos - e->v(1)->_newpos);
		Vec3<double> dp2 = (e->v(0)->_newpos - e->v(1)->_newpos);
		dp1.Normalize();
		dp2.Normalize();
		dp1 *= -_mass / (_mass + _mass) * c_p1p2;
		dp2 *= _mass / (_mass + _mass) * c_p1p2;
		e->v(0)->_newpos += dp1;
		e->v(1)->_newpos += dp2;
	}
}

void Mesh::Bending_Constraint(void)
{
	double stiffness = 0.05;
	int id = 0;
	for (auto e : _edges) {
		if (e->_nbFaces.size() == 2) {
			auto p0 = e->_nbFaces[0]->getOtherVertex(e)->_newpos;
			auto p1 = e->_nbFaces[1]->getOtherVertex(e)->_newpos;
			auto p2 = e->v(0)->_newpos;
			auto p3 = e->v(1)->_newpos;
			Vec3<double> _edge = p3 - p2;
			double length = _edge.Length();
			if (length < 0.001) {
				return;
			}
			double invLength = 1.0 / length;

			Vec3<double> n1 = (p2 - p0).Cross(p3 - p0);
			Vec3<double> n2 = (p3 - p1).Cross(p2 - p1);
			n1 /= n1.LengthSquared();
			n2 /= n2.LengthSquared();

			Vec3<double> d0 = n1 * length;
			Vec3<double> d1 = n2 * length;
			Vec3<double> d2 = n1 * ((p0 - p3).Dot(_edge) * invLength) + n2 * ((p1 - p3).Dot(_edge) * invLength);
			Vec3<double> d3 = n1 * ((p2 - p0).Dot(_edge) * invLength) + n2 * ((p2 - p1).Dot(_edge) * invLength);

			n1.Normalize();
			n2.Normalize();
			double dot = n1.Dot(n2);

			if (dot < -1.0) {
				dot = -1.0;
			}
			if (dot > 1.0) {
				dot = 1.0;
			}

			double phi = acos(dot);

			double lambda = _mass * d0.LengthSquared() + _mass * d1.LengthSquared() 
				+ _mass * d2.LengthSquared() + _mass * d3.LengthSquared();
			if (lambda == 0.0) {
				return;
			}

			lambda = (phi - _DihedralAngle[id++]) / lambda * stiffness;

			if (n1.Cross(n2).Dot(_edge) > 0.0) {
				lambda = -lambda;
			}

			e->_nbFaces[0]->getOtherVertex(e)->_newpos += d0 * (-_mass * lambda);
			e->_nbFaces[1]->getOtherVertex(e)->_newpos += d1 * (-_mass * lambda);
			e->v(0)->_newpos += d2 * (-_mass * lambda);
			e->v(1)->_newpos += d3 * (-_mass * lambda);
		}
	}
}


void Mesh::volume_constraints(void)
{
	double fittness = 1.0;
	double sum = 0.0;
	double lambda = 0.0;
	int i = 0;
	for (auto f : _faces) {
		auto p0 = f->v(0)->_newpos;
		auto p1 = f->v(1)->_newpos;
		auto p2 = f->v(2)->_newpos;
		if (((p1 - p0).Cross(p2 - p0)).Dot(p0) > 0)
		{
			p1 = f->v(2)->_newpos;
			p2 = f->v(1)->_newpos;
		}
		auto c0 = p1.Cross(p2);
		auto c1 = p2.Cross(p0);
		auto c2 = p0.Cross(p1);
		sum += (p0.Cross(p1)).Dot(p2);
		lambda += (c0.LengthSquared() * _mass + c1.LengthSquared() * _mass + c2.LengthSquared() * _mass);
	}

	sum -= fittness * _volume;
	lambda = sum / lambda;

	printf("sum %f \n",sum );
	if (sum > 0) {
		for (auto f : _faces) {
			auto p0 = f->v(0)->_newpos;
			auto p1 = f->v(1)->_newpos;
			auto p2 = f->v(2)->_newpos;
			if (((p1 - p0).Cross(p2 - p0)).Dot(p0) > 0)
			{
				p1 = f->v(2)->_newpos;
				p2 = f->v(1)->_newpos;
			}
			auto c0 = p1.Cross(p2);
			auto c1 = p2.Cross(p0);
			auto c2 = p0.Cross(p1);
			c0 *= (-_mass * lambda);
			c1 *= (-_mass * lambda);
			c2 *= (-_mass * lambda);
			f->v(0)->_newpos += c0;
			f->v(1)->_newpos += c1;
			f->v(2)->_newpos += c2;
		}
	}
}


void Mesh::Simulation(double dt)
{
	ExternalForces(dt);

	int iter = 5;
	for (int k = 0; k < iter; k++) {
		Distance_Constraint();
		Bending_Constraint();
		volume_constraints();
	}

	for (auto v : _vertices) {
		v->_vel = (v->_newpos - v->_pos) / dt;
		v->_pos = v->_newpos;
	}
}

void Mesh::drawPoint(void)
{
	glPushMatrix();
	glEnable(GL_LIGHTING);
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	for (auto v : _vertices) {
		glNormal3f(v->_normal.x(), v->_normal.y(), v->_normal.z());
		glVertex3f(v->x(), v->y(), v->z());
	}
	glEnd();
	glPointSize(1.0f);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void Mesh::drawWireframe(void)
{
	glPushMatrix();
	glEnable(GL_LIGHTING);
	glBegin(GL_LINES);
	for (auto f : _faces) {
		for (int j = 0; j < 3; j++) {
			auto v0 = f->v((j + 1) % 3);
			auto v1 = f->v((j + 2) % 3);
			auto n0 = f->v((j + 1) % 3)->_normal;
			auto n1 = f->v((j + 2) % 3)->_normal;
			glNormal3f(n0.x(), n0.y(), n0.z());
			glVertex3f(v0->x(), v0->y(), v0->z());
			
			glNormal3f(n1.x(), n1.y(), n1.z());
			glVertex3f(v1->x(), v1->y(), v1->z());
		}
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void Mesh::moveToCenter(double scaling)
{
	auto crossVec = _maxPos - _minPos;
	double maxLength = fmax(fmax(crossVec.x(), crossVec.y()), crossVec.z());
	auto center = (_maxPos + _minPos) / 2.0;
	Vec3<double> newCenter(0.0, 0.0, 0.0);

	for (auto v : _vertices) {
		auto pos = v->_pos;
		auto grad = pos - center;
		grad /= maxLength;
		grad *= scaling;
		pos = newCenter;
		pos += grad;
		v->_pos = pos;
		v->_pos0 = pos;
		v->_newpos = pos;
	}
}

void Mesh::reset(void)
{
	for (auto v : _vertices) {
		v->_vel.Clear();
		v->_pos = v->_pos0;
	}
	computeNormal();
}


void Mesh::drawSurface(bool smooth)
{
	glPushMatrix();
	glEnable(GL_LIGHTING);
	smooth == true ? glShadeModel(GL_SMOOTH) : glShadeModel(GL_FLAT);
	for (auto f : _faces) {
		glBegin(GL_TRIANGLES);
		if(!smooth) 
			glNormal3f(f->_normal.x(), f->_normal.y(), f->_normal.z());
		for (int j = 0; j < 3; j++) {
			auto v = f->v(j);
			if (smooth)
				glNormal3f(v->_normal.x(), v->_normal.y(), v->_normal.z());
			glVertex3f(v->x(), v->y(), v->z());
		}
		glEnd();
	}
	glEnable(GL_LIGHTING);
	glPopMatrix();
}


