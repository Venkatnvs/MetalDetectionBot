import React from 'react';
import { SITE_NAME } from '@/constants/Base';
import { LogoImage } from '@/constants/Images';
import { Link } from 'react-router-dom';

const Header = () => {

  return (
    <header className="sticky top-0 z-50 w-full border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
      <div className="container flex h-16 items-center px-4 md:px-6">
        <div className="mr-4 flex items-center gap-2">
          <Link to="/" className="flex items-center gap-2">
            <img
              src={LogoImage}
              alt={SITE_NAME}
              className="h-10 w-10 rounded-md shadow-md me-2"
            />
            <span className="text-xl font-semibold text-primary font-oswald">
              {SITE_NAME}
            </span>
          </Link>
        </div>
        <div className="flex flex-1 items-center justify-between space-x-2 md:justify-end">
          <nav className="flex items-center space-x-2">
            <h2 className="text-sm text-muted-foreground">
              {'Control Page'}
            </h2>
          </nav>
        </div>
      </div>
    </header>
  );
};

export default Header;


