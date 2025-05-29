import React from "react";
import { BrowserRouter, Route, Routes } from "react-router-dom";
import { Toaster } from "./components/ui/sonner";
import MainControlPage from "./pages/ControllPage/MainControlPage";

const App = () => {
  return (
    <>
      <BrowserRouter>
        <Routes>
          <Route>
            <Route
              key="main-control-page"
              path="/"
              element={<MainControlPage />}
            />
          </Route>
          <Route
            path="*"
            element={<p className="text-white-1 flex justify-center items-center h-screen">404 Not Found</p>}
          />
        </Routes>
      </BrowserRouter>
      <Toaster />
    </>
  )
}

export default App
