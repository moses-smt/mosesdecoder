%define name moses
%define version ___RPM_VERSION__
%define release ___RPM_RELEASE__

Name: %{name}
Summary: Moses is a statistical machine translation system that allows you to automatically train translation models for any language pair.
Version: %{version}
Release: %{release}
URL: http://www.statmt.org/%{name}-%{version}/
Source0: %{name}-%{version}.tar.gz
License: LGPL
Group: Development/Tools
Vendor: Capita Translation and Interpreting
Packager: Ian Johnson <ian.johnson@capita-ti.com>
Requires: python >= 2.6, perl >= 5
Prefix: /opt
BuildRoot: %{_builddir}/%{name}-%{version}-%{release}
%description
Moses is a statistical machine translation system that allows you to automatically train translation models for any language pair. All you need is a collection of translated texts (parallel corpus). An efficient search algorithm finds quickly the highest probability translation among the exponential number of choices.
%prep
%setup -q

mkdir -p $RPM_BUILD_ROOT/opt/%{name}-%{version}/giza++-v1.0.7

wget -O $RPM_BUILD_DIR/irstlm-5.70.04.tgz http://moses-suite.googlecode.com/files/irstlm-5.70.04.tgz 
wget -O $RPM_BUILD_DIR/giza-pp-v1.0.7.tgz http://moses-suite.googlecode.com/files/giza-pp-v1.0.7.tar.gz

cd $RPM_BUILD_DIR

tar -zxf irstlm-5.70.04.tgz
tar -zxf giza-pp-v1.0.7.tgz

cd irstlm-5.70.04
bash regenerate-makefiles.sh --force
./configure --prefix $RPM_BUILD_ROOT/opt/%{name}-%{version}/irstlm-5.70.04
make
make install

cd ../giza-pp
make
cp $RPM_BUILD_DIR/giza-pp/GIZA++-v2/GIZA++ $RPM_BUILD_DIR/giza-pp/GIZA++-v2/snt2cooc.out $RPM_BUILD_DIR/giza-pp/mkcls-v2/mkcls $RPM_BUILD_ROOT/opt/%{name}-%{version}/giza++-v1.0.7
%build
./bjam --with-boost=___BOOST_LOCATION__ --with-irstlm=$RPM_BUILD_ROOT/opt/%{name}-%{version}/irstlm-5.70.04 --with-giza=$RPM_BUILD_ROOT/opt/%{name}-%{version}/giza++-v1.0.7 -j2
%install
mkdir -p $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R bin $RPM_BUILD_ROOT/opt/%{name}-%{version}
cp -R scripts/OSM $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/Transliteration $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/analysis $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/ems $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/generic $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/other $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/recaser $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/share $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/tokenizer $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
cp -R scripts/training $RPM_BUILD_ROOT/opt/%{name}-%{version}/scripts
%clean
%files
%defattr(-,root,root)
/opt/%{name}-%{version}/bin/*
/opt/%{name}-%{version}/scripts/OSM/*
/opt/%{name}-%{version}/scripts/Transliteration/*
/opt/%{name}-%{version}/scripts/analysis/*
/opt/%{name}-%{version}/scripts/ems/*
/opt/%{name}-%{version}/scripts/generic/*
/opt/%{name}-%{version}/scripts/other/*
/opt/%{name}-%{version}/scripts/recaser/*
/opt/%{name}-%{version}/scripts/share/*
/opt/%{name}-%{version}/scripts/tokenizer/*
/opt/%{name}-%{version}/scripts/training/*
/opt/%{name}-%{version}/irstlm-5.70.04/*
/opt/%{name}-%{version}/giza++-v1.0.7/*
%pre
if [ "$1" = "1" ]; then
elif [ "$1" = "2" ]; then
  rm $RPM_INSTALL_PREFIX/%{name} 2>/dev/null
fi
%post
ln -s $RPM_INSTALL_PREFIX/%{name}-%{version} $RPM_INSTALL_PREFIX/%{name}
%postun
rm -Rf $RPM_INSTALL_PREFIX/%{name}-%{version} 2>/dev/null
rm $RPM_INSTALL_PREFIX/%{name} 2>/dev/null
