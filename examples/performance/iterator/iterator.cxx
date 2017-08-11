/*
 * Testing performance of an iterator over the mesh
 *
 */

#include <bout.hxx>

#include <chrono>
#include <iostream>
#include <iterator>
#include <time.h>
#include <vector>

// A simple iterator over a 3D set of indices
class MeshIterator
  : public std::iterator< std::forward_iterator_tag, Indices > {
public:
  /// Constructor. This would set ranges. Could depend on thread number
  MeshIterator() : x(0), y(0), z(0), xstart(0), ystart(0), zstart(0) {
    xend = mesh->LocalNx-1;
    yend = mesh->LocalNy-1;
    zend = mesh->LocalNz;
  }

  MeshIterator(int x, int y, int z) : x(x), y(y), z(z), xstart(0), ystart(0), zstart(0) {
    xend = mesh->LocalNx-1;
    yend = mesh->LocalNy-1;
    zend = mesh->LocalNz;
  }

  /// The index variables, updated during loop
  int x, y, z;

  /// Increment operators
  MeshIterator& operator++() { next(); return *this; }
  MeshIterator& operator++(int) { next(); return *this; }

  // Comparison operator
  bool operator!=(const MeshIterator& rhs) const {
    return (x != rhs.x) || (y != rhs.y) || (z != rhs.z);
  }

  // Dereference operator
  Indices operator*() {
    return {x, y, z};
  }

  /// Checks if finished looping. Is this more efficient than
  /// using the more idiomatic it != MeshIterator::end() ?
  bool isDone() const {
    return x > xend;
  }

private:
  int xstart, xend;
  int ystart, yend;
  int zstart, zend;

  /// Advance to the next index
  void next() {
    z++;
    if(z > zend) {
      z = zstart;
      y++;
      if(y > yend) {
        y = ystart;
        x++;
      }
    }
  }
};

///////////////////////////////////////////////////////////////////////////////////
// Vector of single indices

/// A single index over the whole 3D mesh
/// This is just a wrapper around a single integer, with some methods
/// for stencil operations.
///
/// This version includes a pointer to Mesh, which simplifies
/// the stencil operations, and allows checking when multiple meshes are used,
/// but does increase the memory usage and (maybe) efficiency
struct SingleIndex3D_mesh {
  int index; ///< The index in 3D
  Mesh *mesh; ///< The mesh over which we're indexing

  /// Here the offset is a method on the index object
  SingleIndex3D_mesh xp() {
    return { index + mesh->LocalNz * mesh->LocalNy, mesh };
  }
};

using Region_mesh = std::vector<SingleIndex3D_mesh>;

Region_mesh region_mesh(Mesh *mesh) {
  int npoints = mesh->LocalNx * mesh->LocalNy * mesh->LocalNz;
  Region_mesh region(npoints);
  
  for(int i=0;i<npoints;i++) {
    region[i] = {i, mesh};
  }
  return region;
}

///////////////////////////////////////////////////////////////////////////////////
// Vector of single indices, special handling of offsets

/// A single index over the whole 3D mesh
///
/// This version has no Mesh pointer. This reduces memory use,
/// but means that indexing requires a 
struct SingleIndex3D {
  int index; ///< The index in 3D
};

/// This represents an offset from an index, using a Mesh
/// The offset is constructed with three indices,
/// so does not optimise simple cases where some of these indices are zero.
/// For optimisation there is the IndexOffset<> template class below, but that
/// could not easily be passed between functions
///
/// Example:
///
/// IndexOffsetAny xp(1,0,0, mesh);
/// IndexOffsetAny xm(-1,0,0, mesh);
/// for(auto &i : region(mesh) ) {
///    result[i] = f[xp(i)] - f[xm(i)]
/// }
/// 
struct IndexOffsetAny {
  int xo, yo, zo;
  Mesh *mesh;

  SingleIndex3D operator()(const SingleIndex3D &i) {
    return {i.index + zo + mesh->LocalNz * (yo + mesh->LocalNy*xo)};
  }
};

/// This represents an offset from an index, using a Mesh
///
/// Note: Making this offset a template may have performance benefits,
/// but makes passing into/out of functions hard, since each offset is a
/// different type.
/// 
/// Example:
///
/// IndexOffset<1,0,0> xp(mesh);
/// IndexOffset<-1,0,0> xm(mesh);
/// for(auto &i : region(mesh) ) {
///    result[i] = f[xp(i)] - f[xm(i)]
/// }
///
///
template<int xo, int yo, int zo>
struct IndexOffset {
  Mesh *mesh;

  SingleIndex3D operator()(const SingleIndex3D &i) {
    return {i.index + zo + mesh->LocalNz * (yo + mesh->LocalNy*xo)};
  }
};

// Specialised forms for common offsets e.g. xp:
template<>
struct IndexOffset<1,0,0> {
  Mesh *mesh;

  SingleIndex3D operator()(const SingleIndex3D &i) {
    return {i.index + mesh->LocalNz * mesh->LocalNy};
  }
};

/// Define a Region as a vector of indices
using Region = std::vector<SingleIndex3D>;

/// Create a Region over the whole mesh
Region region(Mesh *mesh) {
  int npoints = mesh->LocalNx * mesh->LocalNy * mesh->LocalNz;
  Region reg(npoints);
  
  for(int i=0;i<npoints;i++) {
    reg[i] = {i};
  }
  return reg;
}


///////////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv) {
  BoutInitialise(argc, argv);

  Field3D a = 1.0;
  Field3D b = 2.0;

  Field3D result;
  result.allocate();

  typedef std::chrono::time_point<std::chrono::steady_clock> SteadyClock;
  typedef std::chrono::duration<double> Duration;
  using namespace std::chrono;
  
  // A single loop over block data
  
  BoutReal *ad = &a(0,0,0);
  BoutReal *bd = &b(0,0,0);
  BoutReal *rd = &result(0,0,0);
  
  // Loop over data so first test doesn't have a disadvantage from caching
  for(int i=0;i<10;++i) {
    for(int j=0;j<mesh->LocalNx*mesh->LocalNy*mesh->LocalNz;++j) {
      rd[j] = ad[j] + bd[j];
    }
  }
  
  SteadyClock start1 = steady_clock::now();
  int len = mesh->LocalNx*mesh->LocalNy*mesh->LocalNz;
  for(int i=0;i<10;++i) {
    for(int j=0;j<len;++j) {
      rd[j] = ad[j] + bd[j];
    }
  }
  Duration elapsed1 = steady_clock::now() - start1;

  // Nested loops over block data
  SteadyClock start2 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for(int i=0;i<mesh->LocalNx;++i) {
      for(int j=0;j<mesh->LocalNy;++j) {
        for(int k=0;k<mesh->LocalNz;++k) {
          result(i,j,k) = a(i,j,k) + b(i,j,k);
        }
      }
    }
  }
  Duration elapsed2 = steady_clock::now() - start2;

  // MeshIterator over block data
  SteadyClock start3 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for(MeshIterator i; !i.isDone(); ++i){
      result(i.x,i.y,i.z) = a(i.x,i.y,i.z) + b(i.x,i.y,i.z);
    }
  }
  Duration elapsed3 = steady_clock::now() - start3;

  // DataIterator using begin(), end()
  SteadyClock start4 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for(DataIterator i = std::begin(result), rend=std::end(result); i != rend; ++i){
      result(i.x,i.y,i.z) = a(i.x,i.y,i.z) + b(i.x,i.y,i.z);
    }
  }
  Duration elapsed4 = steady_clock::now() - start4;

  // DataIterator with done()
  SteadyClock start5 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for(DataIterator i = std::begin(result); !i.done() ; ++i){
      result(i.x,i.y,i.z) = a(i.x,i.y,i.z) + b(i.x,i.y,i.z);
    }
  }
  Duration elapsed5 = steady_clock::now() - start5;

  // Range based for DataIterator with indices
  SteadyClock start6 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for(auto i : result){
      result(i.x,i.y,i.z) = a(i.x,i.y,i.z) + b(i.x,i.y,i.z);
    }
  }
  Duration elapsed6 = steady_clock::now() - start6;
  
  // Range based DataIterator 
  SteadyClock start9 = steady_clock::now();
  for (int x=0;x<10;++x) {
    for (const auto &i : result) {
      result[i] = a[i] + b[i];
    }
  }
  Duration elapsed9 = steady_clock::now() - start9;

  // DataIterator over fields
  SteadyClock start10 = steady_clock::now();
  for(int x=0;x<10;x++)
    for(DataIterator d = result.iterator(); !d.done(); d++)
      result[d] = a[d] + b[d];
  Duration elapsed10 = steady_clock::now() - start10;

  // Iterator over vector with mesh pointer
  SteadyClock start11 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for( auto &i : region_mesh(mesh) ) {
      result[i.index] = a[i.index] + b[i.index];
    }
  } 
  Duration elapsed11 = steady_clock::now() - start11;

  // Iterator over vector without mesh pointer
  SteadyClock start12 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for( auto &i : region(mesh) ) {
      result[i.index] = a[i.index] + b[i.index];
    }
  } 
  Duration elapsed12 = steady_clock::now() - start12;

  // Iterator over vector with mesh pointer, not timing vector construction
  auto reg_m = region_mesh(mesh);
  SteadyClock start13 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for( auto &i : reg_m ) {
      result[i.index] = a[i.index] + b[i.index];
    }
  } 
  Duration elapsed13 = steady_clock::now() - start13;

  // Iterator over vector without mesh pointer, not timing vector construction
  auto reg = region(mesh);
  SteadyClock start14 = steady_clock::now();
  for(int x=0;x<10;x++) {
    for( auto &i : reg ) {
      result[i.index] = a[i.index] + b[i.index];
    }
  } 
  Duration elapsed14 = steady_clock::now() - start14;
  
  output << "TIMING\n======\n";
  output << "C loop                     : " << elapsed1.count() << std::endl;
  output << "----- (x,y,z) indexing ----" << std::endl;
  output << "Nested loops               : " << elapsed2.count() << std::endl;
  output << "MeshIterator               : " << elapsed3.count() << std::endl;
  output << "DataIterator (begin/end)   : " << elapsed4.count() << std::endl;
  output << "DataIterator (begin/done)  : " << elapsed5.count() << std::endl;
  output << "C++11 range-based for      : " << elapsed6.count() << std::endl;
  output << "------ [i] indexing -------" << std::endl;
  output << "C++11 Range-based for      : " << elapsed9.count() << std::endl;
  output << "DataIterator (done)        : " << elapsed10.count() << std::endl;
  output << "------ vector of indices --" << std::endl;
  output << "With mesh member           : " << elapsed11.count() << std::endl;
  output << "Without mesh member        : " << elapsed12.count() << std::endl;
  output << "With mesh, no construct    : " << elapsed13.count() << std::endl;
  output << "Without mesh, no construct : " << elapsed14.count() << std::endl;
  

  BoutFinalise();
  return 0;
}
