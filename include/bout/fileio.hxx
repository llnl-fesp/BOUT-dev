///
///
///

class FileIO {
public:
  enum class Mode {CREATE, READ, APPEND};
  
  FileIO() {}

  /// Open a file
  FileIO(const string &filename, Mode mode=Mode::READ);
  
  
  
private:
  
};

class FileData {
public:
  /// Get sub-sections, to allow hierarchical files e.g HDF5
  virtual FileIO* getSection(const string &name) = 0;

  /// Get parents
  virtual FileIO* getParent() = 0;

  /// Test if a variable is available
  bool isSet(const string &key);

  /// Read an integer
  int getInt(const string &key) = 0;
  
  /// Read a BoutReal
  BoutReal getReal(const string &key) = 0;

  /// Read a string
  string getString(const string &key) = 0;
  
  /// Read an integer array
  std::vector<int> getIntArray(const string &key);
  
  /// Read an integer array
  std::vector<BoutReal> getRealArray(const string &key);
  
  /// Read Field2D
  Field2D getField2D(const string &key);

  /// Read Field3D
  Field3D getField3D(const string &key);

protected:

  
  
private:
  
  
};
