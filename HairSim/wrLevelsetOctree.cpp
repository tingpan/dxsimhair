﻿#include "precompiled.h"
#include "wrLevelsetOctree.h"
#include <CGAL\bounding_box.h>
#include "wrGeo.h"

namespace
{
	const unsigned EIDX[12][3] = {
		{ 0, 1, 2 }, { 1, 2, 2 }, { 2, 1, 2 }, { 1, 0, 2 },
		{ 0, 0, 1 }, { 0, 2, 1 }, { 2, 2, 1 }, { 2, 0, 1 },
		{ 0, 1, 0 }, { 1, 2, 0 }, { 2, 1, 0 }, { 1, 0, 0 }
	};

	const unsigned FIDX[6][3] = {
		{ 1, 1, 2 }, { 0, 1, 1 }, { 1, 2, 1 },
		{ 2, 1, 1 }, { 1, 0, 1 }, { 1, 1, 0 }
	};

	const unsigned BIDX[8][2] = {
		{ 12, 20 }, { 21, 9 }, { 26, 2 }, { 24, 10 },
		{ 4, 26 }, { 16, 22 }, { 25, 14 }, { 19, 23 }
	};

	const unsigned CIDX[8][8] = {
		{ 0, 8, 20, 11, 12, 21, 26, 24 },
		{ 8, 1, 9, 20, 21, 13, 22, 26 },
		{ 20, 9, 2, 10, 26, 22, 14, 23 },
		{ 11, 20, 10, 3, 24, 26, 23, 15 },
		{ 12, 21, 26, 24, 4, 16, 25, 19 },
		{ 21, 13, 22, 26, 16, 5, 17, 25 },
		{ 26, 22, 14, 23, 25, 17, 6, 18 },
		{ 24, 26, 23, 15, 19, 25, 18, 7 }
	};
}


wrLevelsetOctree::wrLevelsetOctree()
{
}


wrLevelsetOctree::~wrLevelsetOctree()
{
	release();
}


bool wrLevelsetOctree::construct(Polyhedron_3& geom, size_t maxLvl)
{
	release();
	nMaxLevel = maxLvl;

	// contruct all the triangles
	nTriangles = geom.size_of_facets();
	triList = new Triangle_3[nTriangles];

	size_t count = 0;
	for (auto tItr = geom.facets_begin(); tItr != geom.facets_end(); tItr++, count++)
	{
		if (!tItr->is_triangle())
			throw std::exception("Not a triangle");

		auto ffItr = tItr->facet_begin();
		Point_3 vP[3];
		for (size_t i = 0; i < 3; i++)
			vP[i] = ffItr->vertex()->point();

		triList[count] = Triangle_3(vP[0], vP[1], vP[2]);
		triList[count].fh = tItr;
		tItr->idx = count;
		triList[count].initInfo();
	}

	Node *root = createRootNode(geom);

	constructChildren(root);
	computeGradient();

	return true;
}

void wrLevelsetOctree::constructChildren(Node* root)
{

	//   /----0----/            •-----y
	//  3|        1|            |
	// /----2----/ |            |
	// | 4       | 5            x
	// | |       | |              1
	// 7 /----8--6-/          4  0(5)  2  
	// |11       |9               3
	// /----10---/

	// 顶点0-7，边中点8-19，面心20-25，体心26
	// 子节点方位标记和顶点方位标记同
	DT_3::Vertex_handle v[27];
	for (size_t i = 0; i < 8; i++)
		v[i] = root->vertices[i];

	vec3 coords[3];

	coords[0][0] = root->bbox.xmin();
	coords[0][1] = (root->bbox.xmin() + root->bbox.xmax()) / 2.0f;
	coords[0][2] = root->bbox.xmax();

	coords[1][0] = root->bbox.ymin();
	coords[1][1] = (root->bbox.ymin() + root->bbox.ymax()) / 2.0f;
	coords[1][2] = root->bbox.ymax();

	coords[2][0] = root->bbox.zmin();
	coords[2][1] = (root->bbox.zmin() + root->bbox.zmax()) / 2.0f;
	coords[2][2] = root->bbox.zmax();

	const unsigned OE = 8;
	for (size_t i = 0; i < 12; i++)
		v[i + OE] = dt.insert(Point_3(coords[0][EIDX[i][0]], coords[1][EIDX[i][1]], coords[2][EIDX[i][2]]), root->vertices[0]);
	
	const unsigned OF = 20;
	for (size_t i = 0; i < 6; i++)
		v[i + OF] = dt.insert(Point_3(coords[0][FIDX[i][0]], coords[1][FIDX[i][1]], coords[2][FIDX[i][2]]), root->vertices[0]);

	v[26] = dt.insert(Point_3(coords[0][1], coords[1][1], coords[2][1]), root->vertices[0]);

	// deploy chidren
	for (size_t i =  0; i < 8; i++)
	{
		Node* child = createNode();
		child->level = root->level + 1;
		child->pParent = root;

		for (size_t j = 0; j < 8; j++)
			child->vertices[j] = v[CIDX[i][j]];

		child->bbox = Cube(v[BIDX[i][0]]->point(), v[BIDX[i][1]]->point());
		computeTripleFromBbox(child->triple, child->bbox);

		for (auto eItr = root->eList.begin(); eItr != root->eList.end(); eItr++)
		{
			if (CGAL::do_intersect(triList[*eItr], child->triple.bbox()))
				child->eList.push_back(*eItr);
		}

		// if has elements and not max level, split
		if (!child->eList.empty() || child->level >= nMaxLevel)
			constructChildren(child);
		else computeMinDistance(child);

		root->children[i] = child;
	}

}

void wrLevelsetOctree::release()
{
	releaseExceptDt();
	dt.clear();
}

void wrLevelsetOctree::releaseExceptDt()
{
	size_t n = cellList.size();
	for (size_t i = 0; i < n; i++)
		delete cellList[i];

	SAFE_DELETE_ARRAY(triList);
	nTriangles = 0;

	pRoot = nullptr;
	nMaxLevel = 0;
	cellList.clear();
}

void wrLevelsetOctree::computeGradient()
{
	assert(0);
}

void wrLevelsetOctree::computeMinDistance(Node* node)
{
	for (size_t i = 0; i < 8; i++)
	{
		auto& vh = node->vertices[i];
		auto& dist = vh->info().minDist;
		if (dist < 1.0e8) continue;

		int type = -1;
		size_t triIdx = 0;
		Node* curNode = node;
		Vector_3 diff;

		while (true)
		{
			if (!curNode) assert(0);

			if (!curNode->eList.empty())
			{
				double tmpDist = minDist(vh->point(), curNode->eList.cbegin(), curNode->eList.cend(), &diff, &triIdx, &type);
				double distLimit = minDist(curNode->bbox, vh->point());

				if (tmpDist < distLimit)
				{
					dist = tmpDist;
					break;
				}
			}
			curNode = node->pParent;
		}

		// determine the sign
		switch (type)
		{
		case 0:
			dist *= detSignOnFace(vh->point(), diff, triIdx);
			break;
		case 1:
			dist *= detSignOnEdge(vh->point(), diff, triIdx, 2);
			break;
		case 3:
			dist *= detSignOnEdge(vh->point(), diff, triIdx, 0);
			break;
		case 5:
			dist *= detSignOnEdge(vh->point(), diff, triIdx, 2);
			break;
		case 2:
			dist *= detSignOnVertex(vh->point(), diff, triIdx, 2);
			break;
		case 4:
			dist *= detSignOnVertex(vh->point(), diff, triIdx, 0);
			break;
		case 6:
			dist *= detSignOnVertex(vh->point(), diff, triIdx, 1);
			break;
		default:
			throw std::exception("unexpected type.");
		}
	}
}

int wrLevelsetOctree::detSignOnFace(const Point_3& p, const Vector_3& diff, size_t triIdx)
{
	if (diff * triList[triIdx].normal > 0.0) return 1;
	else return -1;
}

int wrLevelsetOctree::detSignOnEdge(const Point_3& p, const Vector_3& diff, size_t triIdx, int seq)
{
	auto edge = triList[triIdx].fh->facet_begin();
	for (size_t i = 0; i < seq; i++, edge++);

	Vector_3 normal = (triList[edge->facet()->idx].normal + triList[triIdx].normal) / 2.0;
	if (diff * normal > 0.0) return 1;
	else return -1;
}

int wrLevelsetOctree::detSignOnVertex(const Point_3& p, const Vector_3& diff, size_t triIdx, int seq)
{
	auto edge = triList[triIdx].fh->facet_begin();
	for (size_t i = 0; i < seq; i++, edge++);

	auto vh = edge->vertex();
	const size_t dim = vh->degree();
	auto vc = vh->vertex_begin();
	Vector_3 normal(0, 0, 0);
	for (size_t i = 0; i < dim; i++, vc++)
		normal = normal + triList[vc->face()->idx].normal;

	normal = normal / dim;
	if (diff * normal > 0.0) return 1;
	else return -1;
}


// begin != end !!!
template <class Iterator>
double wrLevelsetOctree::minDist(const Point_3& p, Iterator begin, Iterator end, Vector_3* diff, size_t* pTriIdx, int* pType)
{
	assert(begin != end);

	double dist = 1.e8;
	size_t triIdx = 0;
	int type = 0;
	double s;
	double t;

	for (auto eItr = begin; eItr != begin; eItr++)
	{
		WRG::PointTriangleDistResult<K::FT> res;
		auto &tri = triList[*eItr];
		tri.computeInfo(p);
		WRG::distance(p, tri, tri.infos, res);
		if (dist > res.dist)
		{
			dist = res.dist;
			type = res.type;
			s = res.s;
			t = res.t;
			triIdx = *eItr;
		}
	}
	if (diff)
		*diff = p - (triList[triIdx].vertex(0) + s * triList[triIdx].E0 + t * triList[triIdx].E1);

	if (pTriIdx) *pTriIdx = triIdx;
	if (pType) *pType = type;
	return dist;
}

double wrLevelsetOctree::minDist(const Cube& bbox, const Point_3& p)
{
	double dist = 1.e9;

#define MIN_TEST(d, d0)\
	{if ((d) < (d0))\
		(d0) = (d);}

	MIN_TEST(fabs(bbox.xmax() - p.x()), dist);
	MIN_TEST(fabs(bbox.xmin() - p.x()), dist);
	MIN_TEST(fabs(bbox.ymax() - p.y()), dist);
	MIN_TEST(fabs(bbox.ymin() - p.y()), dist);
	MIN_TEST(fabs(bbox.zmax() - p.z()), dist);
	MIN_TEST(fabs(bbox.zmin() - p.z()), dist);

	return dist;
}

wrLevelsetOctree::Node* wrLevelsetOctree::createRootNode(const Polyhedron_3& geom)
{
	Node* node = createNode();

	Cube bbox = CGAL::bounding_box(geom.points_begin(), geom.points_end());
	WRG::enlarge(bbox, 1.5); // 不希望贴的太紧
	node->vertices[0] = dt.insert(Point_3(bbox.xmin(), bbox.ymin(), bbox.zmax()));
	node->vertices[1] = dt.insert(Point_3(bbox.xmin(), bbox.ymax(), bbox.zmax()));
	node->vertices[2] = dt.insert(Point_3(bbox.xmax(), bbox.ymax(), bbox.zmax()));
	node->vertices[3] = dt.insert(Point_3(bbox.xmax(), bbox.ymin(), bbox.zmax()));

	node->vertices[4] = dt.insert(Point_3(bbox.xmin(), bbox.ymin(), bbox.zmin()));
	node->vertices[5] = dt.insert(Point_3(bbox.xmin(), bbox.ymax(), bbox.zmin()));
	node->vertices[6] = dt.insert(Point_3(bbox.xmax(), bbox.ymax(), bbox.zmin()));
	node->vertices[7] = dt.insert(Point_3(bbox.xmax(), bbox.ymin(), bbox.zmin()));

	node->level = 0;
	node->bbox = bbox;

	computeTripleFromBbox(node->triple, bbox);

	for (size_t i = 0; i < nTriangles; i++)
		node->eList.push_back(i);

	return node;
}

void wrLevelsetOctree::computeTripleFromBbox(Cube& triple, const Cube& bbox) const
{
	K::Vector_3 radius = (bbox.max() - bbox.min()) / 2.0f;
	Point_3 center = bbox.min() + radius / 2.0f;
	triple = Cube(center - 3.0f * radius, center + 3.0f * radius);
}

wrLevelsetOctree::Node* wrLevelsetOctree::createNode()
{
	Node* node = new Node;
	memset(node->children, 0, sizeof(node->children));
	cellList.push_back(node);
	return node;
}


float wrLevelsetOctree::queryDistance(const Point_3& p) const
{
	if (!dt.number_of_cells()) return -1.0f;

	if (pRoot->bbox.bounded_side(p) != CGAL::ON_BOUNDED_SIDE)
	{
		// do nothing
		return 1.e9;
	}
	else
	{
		DT_3::Cell_handle ch = dt.locate(p);

		Point_3 v[4];
		for (size_t i = 0; i < 4; i++)
			v[i] = ch->vertex(i)->point();

		assert(CGAL::volume(v[0], v[1], v[2], v[3]) > 0);
		float vol[4];
		vol[0] = CGAL::volume(v[1], v[3], v[2], p);
		vol[1] = CGAL::volume(v[2], v[3], v[0], p);
		vol[2] = CGAL::volume(v[0], v[3], v[1], p);
		vol[3] = CGAL::volume(v[0], v[1], v[2], p);

		float sum = 0.f;
		float numer = 0.0f;
		for (size_t i = 0; i < 4; i++)
		{
			sum += vol[i];
			numer += vol[i] * ch->vertex(i)->info().minDist;
		}

		return numer / sum;
	}
}